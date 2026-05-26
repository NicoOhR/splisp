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
  size_t depth = 0;
  void add_instruction(ISA::Operation op,
                       std::optional<uint64_t> operand = std::nullopt);
  void emit_top(const core::Top &top);
  void emit_expr(const core::Expr &expr);
  void emit_top_define(const core::Define &def);
  void emit_cond(const core::Cond &cond);
  void emit_lambda(const core::Lambda &lambda);
  void emit_apply(const core::Apply &application);
  void emit_var(const core::Var &variable);
  void emit_const(const core::Const &const_var);
  void emit_set(const core::Set &set_op);

  // function builtins: used by emit_apply — emits args then the opcode
  const std::map<core::SymbolId, ISA::Operation> builtins = {
      {0, ISA::Operation::ADD},    {1, ISA::Operation::SUB},
      {2, ISA::Operation::MUL},    {3, ISA::Operation::DIV},
      {4, ISA::Operation::MOD},    {5, ISA::Operation::CONS},
      {6, ISA::Operation::CAR},    {7, ISA::Operation::CDR},
      {9, ISA::Operation::ISNULL},
      {10, ISA::Operation::EQ},
      {11, ISA::Operation::LT},
      {12, ISA::Operation::LE},
      {13, ISA::Operation::GE},
      {14, ISA::Operation::GT},
  };
  // constant builtins: used by emit_var — emits a single no-operand push
  const std::map<core::SymbolId, ISA::Operation> const_builtins = {
      {8, ISA::Operation::PUSHNIL},
  };
};

void print_bytecode(const std::vector<ISA::Instruction> &bytecode);
