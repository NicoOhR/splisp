#include <algorithm>
#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <exception>
#include <frontend/ast.hpp>
#include <frontend/core.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>

using ISA::Instruction;

std::vector<ISA::Instruction> Generator::generate() {
  for (auto &top : this->program) {
    emit_top(top);
  }
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::HALT, .operand = std::nullopt});
  return this->bytecode;
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
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::CJMP, .operand = std::nullopt});
  emit_expr(*cond.otherwise);
  size_t jmp_idx = this->bytecode.size();
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::JMP, .operand = std::nullopt});
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
          this->bytecode.push_back(
              ISA::Instruction{.op = ISA::Operation::PUSH, .operand = 0});
        }
      },
      expr.node);
}

void Generator::emit_const(const core::Const &const_var) {
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::PUSH, .operand = const_var.value});
};

void Generator::emit_top_define(const core::Define &def) {
  // function which defines top level (global) defintion
  Generator::emit_expr(*def.rhs);
  this->global_symbols.push_back(def.name);
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::MKGLOBAL, .operand = def.name});
};

void Generator::emit_lambda(const core::Lambda &lambda) {
  // JMP        #jump to MKCLOSURE
  // ENTER
  // emit<expr> #generate the body
  // LEAVE; RET #function clean up
  // MKCLOSURE  #capture frame, pointing to ENTER

  // save a copy of the local symbols
  auto saved_locals = this->local_symbols;
  for (size_t i = 0; i < lambda.formals.size(); i++) {
    const auto symbol_id = *lambda.formals.at(i);
    this->local_symbols[symbol_id] = i;
  }

  const auto jmp_idx = this->bytecode.size();
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::JMP, .operand = std::nullopt});
  const auto enter_offset = this->bytecode.size() * 9;
  this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::ENTER,
                                            .operand = lambda.formals.size()});
  for (auto &&expr : lambda.body) {
    Generator::emit_expr(*expr);
  }
  this->bytecode.push_back(
      Instruction{.op = ISA::Operation::LEAVE, .operand = std::nullopt});
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::RET, .operand = std::nullopt});
  const auto mk_offset = this->bytecode.size();
  this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::MKCLOSURE,
                                            .operand = enter_offset});
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
      this->bytecode.push_back(
          ISA::Instruction{.op = it->second, .operand = std::nullopt});
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
  this->bytecode.push_back(ISA::Instruction{
      .op = ISA::Operation::CALL, .operand = application.args.size()});
};
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

void Generator::emit_var(const core::Var &variable) {
  if (std::find(this->global_symbols.begin(), this->global_symbols.end(),
                variable.id) != this->global_symbols.end()) {
    this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::LOADGLOBAL,
                                              .operand = variable.id});
  } else if (this->local_symbols.find(variable.id) !=
             this->local_symbols.end()) {
    this->bytecode.push_back(
        ISA::Instruction{.op = ISA::Operation::GETLOCAL,
                         .operand = this->local_symbols[variable.id]});
  } else {
    std::cerr << "Variable not found" << std::endl;
  }
};

void Generator::emit_set(const core::Set &set_op) {
  emit_expr(*set_op.rhs);
  if (this->local_symbols.find(set_op.name) != this->local_symbols.end()) {
    this->bytecode.push_back(
        ISA::Instruction{.op = ISA::Operation::SETLOCAL,
                         .operand = this->local_symbols.at(set_op.name)});
  } else if (std::find(this->global_symbols.begin(), this->global_symbols.end(),
                       set_op.name) != this->global_symbols.end()) {
    this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::MUTGLOBAL,
                                              .operand = set_op.name});
  } else {
    std::cerr << "Variable not found for set!" << std::endl;
  }
}
