#include <algorithm>
#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <cstdint>
#include <exception>
#include <frontend/ast.hpp>
#include <frontend/core.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>

using ISA::Instruction;

void print_bytecode(const std::vector<ISA::Instruction> &bytecode) {
  for (size_t i = 0; i < bytecode.size(); i++) {
    const auto &spec = ISA::spec_list[static_cast<uint8_t>(bytecode[i].op)];
    std::cerr << "[" << i * 9 << "] " << spec.mnemonic;
    if (bytecode[i].operand.has_value()) {
      std::cerr << " " << bytecode[i].operand.value();
    }
    std::cerr << "\n";
  }
}

std::vector<ISA::Instruction> Generator::generate() {
  for (auto &top : this->program) {
    emit_top(top);
  }
  add_instruction(ISA::Operation::HALT, std::nullopt);
  return this->bytecode;
}

void Generator::add_instruction(ISA::Operation op,
                                std::optional<uint64_t> operand) {
  this->bytecode.push_back(ISA::Instruction{op, operand});
}

void Generator::emit_top(const core::Top &top) {
  std::visit(
      [this](const auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, core::Define>) {
          emit_top_define(p);
        } else if constexpr (std::is_same_v<T, core::Expr>) {
          emit_expr(p);
        }
      },
      top);
}

void Generator::emit_cond(const core::Cond &cond) {
  emit_expr(*cond.condition);
  size_t cjmp_idx = this->bytecode.size();
  add_instruction(ISA::Operation::CJMP, std::nullopt);
  emit_expr(*cond.otherwise);
  size_t jmp_idx = this->bytecode.size();
  add_instruction(ISA::Operation::JMP, std::nullopt);
  this->bytecode[cjmp_idx].operand = this->bytecode.size() * 9;
  emit_expr(*cond.then);
  this->bytecode[jmp_idx].operand = this->bytecode.size() * 9;
}

void Generator::emit_expr(const core::Expr &expr) {
  std::visit(
      [this](const auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, core::Apply>) {
          Generator::emit_apply(p);
        }
        if constexpr (std::is_same_v<T, core::Lambda>) {
          Generator::emit_lambda(p);
        }
        if constexpr (std::is_same_v<T, core::Const>) {
          Generator::emit_const(p);
        }
        if constexpr (std::is_same_v<T, core::Cond>) {
          Generator::emit_cond(p);
        }
        if constexpr (std::is_same_v<T, core::Var>) {
          Generator::emit_var(p);
        }
        if constexpr (std::is_same_v<T, core::Set>) {
          Generator::emit_set(p);
        }
        if constexpr (std::is_same_v<T, core::Undef>) {
          add_instruction(ISA::Operation::PUSH, 0);
        }
      },
      expr.node);
}

void Generator::emit_const(const core::Const &const_var) {
  add_instruction(ISA::Operation::PUSH, const_var.value);
};

void Generator::emit_top_define(const core::Define &def) {
  // function which defines top level (global) defintion
  // critically, the global symbol has to be registered before the rhs side is
  // emitted for recrusion
  this->global_symbols.push_back(def.name);
  Generator::emit_expr(*def.rhs);
  add_instruction(ISA::Operation::MKGLOBAL, def.name);
};

void Generator::emit_lambda(const core::Lambda &lambda) {
  // JMP        #jump to MKCLOSURE
  // ENTER
  // emit<expr> #generate the body
  // NROT
  // DROP
  // RET
  // MKCLOSURE  #capture frame, pointing to ENTER

  // save a copy of the local symbols to be restored after this level is
  // finished generating
  auto saved_locals = this->local_symbols;
  auto n = lambda.formals.size() + saved_locals.size();

  // add new formals to the local variables list
  for (size_t i = 0; i < lambda.formals.size(); i++) {
    const auto symbol_id = *lambda.formals.at(i);
    this->local_symbols[symbol_id] = i;
  }
  // in the local symbols map, change the newly captured formals (now in
  // local_symbols) indecies to match with the inner layout
  for (auto &[sym_id, outer_idx] : saved_locals)
    this->local_symbols[sym_id] = lambda.formals.size() + outer_idx;

  const auto jmp_idx = this->bytecode.size();
  add_instruction(ISA::Operation::JMP, std::nullopt);
  const auto enter_offset = this->bytecode.size() * 9;
  add_instruction(ISA::Operation::ENTER, n);
  for (size_t i = 0; i < lambda.body.size() - 1; i++) {
    auto &&expr = lambda.body.at(i);
    Generator::emit_expr(*expr);
    add_instruction(ISA::Operation::DROP, 1);
  }
  Generator::emit_expr(*lambda.body.back());
  // rotate the result down to the bottom of the frame and drop the scratch
  // variables that it calculates
  add_instruction(ISA::Operation::NROT, n + 1);
  add_instruction(ISA::Operation::DROP, n);
  add_instruction(ISA::Operation::RET, std::nullopt);
  const auto mk_offset = this->bytecode.size();
  add_instruction(ISA::Operation::MKCLOSURE, enter_offset);
  this->bytecode[jmp_idx].operand = mk_offset * 9;
  // restore for when called a level up.
  this->local_symbols = saved_locals;
};
void Generator::emit_apply(const core::Apply &application) {
  if (auto *var = std::get_if<core::Var>(&application.callee->node)) {
    auto it = this->builtins.find(var->id);
    if (it != this->builtins.end()) {
      for (auto &&arg : application.args)
        emit_expr(*arg);
      add_instruction(it->second, std::nullopt);
      return;
    }
    emit_var(*var); // push handle, fall through to args + CALL
  } else if (auto *lam = std::get_if<core::Lambda>(&application.callee->node)) {
    emit_lambda(*lam);
  } else if (auto *app = std::get_if<core::Apply>(&application.callee->node)) {
    emit_apply(*app);
  } else {
    emit_expr(*application.callee);
  }
  for (auto &&arg : application.args)
    emit_expr(*arg);
  add_instruction(ISA::Operation::CALL, application.args.size());
};
void Generator::emit_var(const core::Var &variable) {
  // constant builtins (nil etc.) emit a single opcode with no operand
  if (auto it = this->const_builtins.find(variable.id);
      it != this->const_builtins.end()) {
    add_instruction(it->second, std::nullopt);
    return;
  }
  if (std::find(this->global_symbols.begin(), this->global_symbols.end(),
                variable.id) != this->global_symbols.end()) {
    add_instruction(ISA::Operation::LOADGLOBAL, variable.id);
  } else if (this->local_symbols.find(variable.id) !=
             this->local_symbols.end()) {
    add_instruction(ISA::Operation::GETLOCAL, this->local_symbols[variable.id]);
  } else {
    std::cerr << "Variable not found" << std::endl;
  }
};

void Generator::emit_set(const core::Set &set_op) {
  emit_expr(*set_op.rhs);
  if (this->local_symbols.find(set_op.name) != this->local_symbols.end()) {
    add_instruction(ISA::Operation::SETLOCAL,
                    this->local_symbols.at(set_op.name));
  } else if (std::find(this->global_symbols.begin(), this->global_symbols.end(),
                       set_op.name) != this->global_symbols.end()) {
    add_instruction(ISA::Operation::MUTGLOBAL, set_op.name);
  } else {
    std::cerr << "Variable not found for set!" << std::endl;
  }
}
