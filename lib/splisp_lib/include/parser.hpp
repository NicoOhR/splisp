#pragma once

#include <algorithm>
#include <ast.hpp>
#include <cstdint>
#include <lexer.hpp>
#include <memory>
#include <variant>

using namespace ast;

class Parser {
public:
  Parser(Lexer lex);
  AST parse();

private:
  std::unique_ptr<SExp> create_sexp();
  List create_list();

  Lexer lex;
  std::optional<Keyword> is_keyword(std::string str);
  std::optional<bool> is_bool(std::string str);
  std::optional<uint64_t> is_number(std::string str);

  const std::unordered_map<std::string, Keyword> kwords = {
      {"if", Keyword::if_expr},
      {"lambda", Keyword::lambda},
      {"define", Keyword::define}};
};
