#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <cstdint>
#include <frontend/ast.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

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
          emit_define(p);
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

void Generator::emit_const(const core::Const &const_var) {};
void Generator::emit_define(const core::Define &def) {};
void Generator::emit_lambda(const core::Lambda &lambda) {};
void Generator::emit_apply(const core::Apply &application) {};
void Generator::emit_var(const core::Var &variable) {};
