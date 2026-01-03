#pragma once

#include <backend/isa/isa.hpp>
#include <frontend/ast.hpp>
#include <optional>
#include <string>

class Generator {
public:
  Generator(ast::AST &ast) : ast(ast) {};
  void generate();

private:
  friend struct GeneratorTestAccess;
  const ast::AST &ast;
  std::vector<ISA::Instruction> program;
  void lower_sexp(const ast::SExp &sexp);
  void lower_symbol(const ast::Symbol &sym);
  void lower_keyword(const ast::List &list);
  void lower_function(const ast::List &list);
  void lower_list(const ast::List &list);
};
