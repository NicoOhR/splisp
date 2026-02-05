#pragma once

#include <cstdint>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <memory>
#include <string>

using namespace ast;

class Parser {
public:
  Parser(Lexer lex);
  AST parse();

private:
  std::unique_ptr<SExp> create_sexp();
  List create_list();
  void resolve_forms(SExp &rsexp, bool is_top_level);
  ast::List create_define(List &list);
  ast::List create_lambda(List &list);
  ast::SExp create_let(List &list);

  Lexer lex;
  std::optional<bool> is_bool(std::string str);
  std::optional<Keyword> is_keyword(std::string str);
  std::optional<uint64_t> is_number(std::string str);

  const std::unordered_map<std::string, Keyword> kwords = {
      {"if", Keyword::if_expr},
      {"let", Keyword::let},
      {"lambda", Keyword::lambda},
      {"define", Keyword::define}};
};
