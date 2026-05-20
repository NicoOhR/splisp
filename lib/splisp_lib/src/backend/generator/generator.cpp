#include <algorithm>
#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <exception>
#include <frontend/ast.hpp>
#include <frontend/core.hpp>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <variant>

using ISA::Instruction;

void Generator::generate() {
  for (auto &top : this->program) {
    emit_top(top);
  }
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
  // List<Sexp(Keyword(if)), Sexp(cond), Sexp(then), Sexp(else)>
  // 0. emit PUSH X (will calculate X)
  // 1. emit condition to program vector
  // 2. emit CJMP (note length here)
  // 3. emit (else)
  // 4. emit JMP Y (will calculate Y)
  // 5. emit (then)
  // X = length before (5), Y = length after (5)
  this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::PUSH});
  size_t cjmp_idx = this->bytecode.size() - 1; // note the index of jump address
  emit_expr(*cond.condition);
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::CJMP, .operand = std::nullopt});
  emit_expr(*cond.otherwise);
  this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::PUSH});
  size_t jmp_idx = this->bytecode.size() - 1;
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::JMP, .operand = std::nullopt});
  size_t X = (jmp_idx + 2) * 9;
  emit_expr(*cond.then);
  size_t Y = (this->bytecode.size()) * 9;
  this->bytecode[cjmp_idx].operand = X;
  this->bytecode[jmp_idx].operand = Y;
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
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::RET, .operand = 0});
};

void Generator::emit_lambda(const core::Lambda &lambda) {
  // JMP        #jump to MKCLOSURE
  // ENTER
  // emit<expr> #generate the body
  // LEAVE; RET #function clean up
  // MKCLOSURE  #capture frame, pointing to ENTER

  for (int i = 0; i < lambda.formals.size(); i++) {
    const auto symbol_id = *lambda.formals.at(i);
    this->local_symbols[symbol_id] = i;
  }

  const auto jmp_idx = this->bytecode.size();
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::JMP, .operand = 0});
  const auto enter_idx = this->bytecode.size();
  this->bytecode.push_back(ISA::Instruction{.op = ISA::Operation::ENTER,
                                            .operand = lambda.formals.size()});
  for (auto &&expr : lambda.body) {
    Generator::emit_expr(*expr);
  }
  this->bytecode.push_back(
      Instruction{.op = ISA::Operation::LEAVE, .operand = 0});
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::RET, .operand = 0});
  const auto mk_idx = this->bytecode.size();
  this->bytecode.push_back(
      ISA::Instruction{.op = ISA::Operation::MKCLOSURE, .operand = enter_idx});
  this->bytecode[jmp_idx].operand = mk_idx;
};
void Generator::emit_apply(const core::Apply &application) {};
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
