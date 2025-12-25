#pragma once

#include <cstdint>
#include <lexer.hpp>
#include <memory>
#include <variant>
#include <vector>

struct Symbol {
  std::variant<std::string, std::uint64_t, bool> value;
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
  void print_sexp(const SExp &sexp) const;
  std::unique_ptr<SExp> create_sexp(Lexer &lex);
  List create_list(Lexer &lex);
  std::vector<std::unique_ptr<SExp>> AST;
};
