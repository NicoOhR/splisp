#pragma once

#include <cstdint>
#include <lexer.hpp>
#include <memory>
#include <variant>
#include <vector>

enum Keyword { if_expr, lambda, define };

struct Symbol {
  // string variant primarily for debugging
  std::variant<Keyword, std::string, std::uint64_t, bool> value;
};

struct SExp;

struct List {
  std::vector<std::unique_ptr<SExp>> list;
};

struct SExp {
  std::variant<List, Symbol> node;
};

class Parser {
public:
  Parser(Lexer &lex);
  void print_ast() const;

private:
  void print_sexp(const SExp &sexp, int level) const;
  void print_symbol(const Symbol &sym, int level) const;

  std::unique_ptr<SExp> create_sexp(Lexer &lex);
  List create_list(Lexer &lex);
  std::vector<std::unique_ptr<SExp>> AST;

  std::optional<Keyword> is_keyword(std::string str);
  std::optional<bool> is_bool(std::string str);
  std::optional<uint64_t> is_number(std::string str);

  const std::unordered_map<std::string, Keyword> kwords = {
      {"if", Keyword::if_expr},
      {"lambda", Keyword::lambda},
      {"define", Keyword::define}};
};
