#pragma once

#include <backend/isa/isa.hpp>
#include <frontend/core.hpp>
#include <optional>
#include <string>

class Generator {
public:
  Generator(core::Program &program) : program(program) {};
  void generate();

private:
  friend struct GeneratorTestAccess;
  const core::Program &program;
  std::vector<ISA::Instruction> bytecode;
  void emit_top(const core::Top &top);
  void emit_expr(const core::Expr &expr);
  void emit_define(const core::Define &def);
  void emit_cond(const core::Cond &cond);
  void emit_lambda(const core::Lambda &lambda);
  void emit_apply(const core::Apply &application);
  void emit_var(const core::Var &variable);
  void emit_const(const core::Const &const_var);
};
