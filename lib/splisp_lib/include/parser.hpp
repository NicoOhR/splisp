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
  std::vector<std::unique_ptr<SExp>> List;
};

struct SExp {
  std::variant<List, Symbol> node;
};

class Parser {
public:
  Parser(Lexer &lex);

private:
  std::vector<std::unique_ptr<SExp>> AST;
};
