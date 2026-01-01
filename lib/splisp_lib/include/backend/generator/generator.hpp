#pragma once

#include <backend/isa/isa.hpp>
#include <frontend/ast.hpp>
#include <optional>

class Generator {
public:
  Generator(ast::AST ast) : ast(std::move(ast)) {};
  std::vector<ISA::Instruction> generate();

private:
  ast::AST ast;
  std::vector<ISA::Instruction> lower_sexp(ast::SExp sexp);
  ISA::Instruction emit(ISA::Operation op, std::optional<uint64_t> operand);
};
