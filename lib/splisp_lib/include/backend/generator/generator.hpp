#pragma once

#include "frontend/ast.hpp"
#include <backend/isa/isa.hpp>
#include <frontend/core.hpp>
#include <map>
#include <optional>
#include <string>

class Generator {
public:
  Generator(const core::Program &program) : program(program) {};
  std::vector<ISA::Instruction> generate();

private:
  friend struct GeneratorTestAccess;
  const core::Program &program;
  std::vector<core::SymbolId> global_symbols;
  // symbol name -> index in the formals list/code env
  std::map<core::SymbolId, size_t> local_symbols;
  std::vector<ISA::Instruction> bytecode;
  void emit_top(const core::Top &top);
  void emit_expr(const core::Expr &expr);
  void emit_top_define(const core::Define &def);
  void emit_cond(const core::Cond &cond);
  void emit_lambda(const core::Lambda &lambda);
  void emit_apply(const core::Apply &application);
  void emit_var(const core::Var &variable);
  void emit_const(const core::Const &const_var);
  void emit_set(const core::Set &set_op);

  const std::map<core::SymbolId, ISA::Operation> builtins = {
      {0, ISA::Operation::ADD}, {1, ISA::Operation::SUB},
      {2, ISA::Operation::MUL}, {3, ISA::Operation::DIV},
      {4, ISA::Operation::MOD},
  };
};

void print_bytecode(const std::vector<ISA::Instruction> &bytecode);
