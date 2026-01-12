#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ast {
enum Keyword { if_expr, let, lambda, define };

struct SExp;

struct Symbol {
  std::variant<Keyword, std::string, std::uint64_t, bool> value;
};

struct List {
  std::vector<std::unique_ptr<SExp>> list;
};

struct SExp {
  std::variant<List, Symbol> node;
};

using AST = std::vector<std::unique_ptr<SExp>>;

std::optional<std::string> to_string(const SExp &sexp);
void print_ast(const AST &ast);
void print_sexp(const SExp &sexp, int level);
void print_symbol(const Symbol &sym, int level);
std::string print_keyword(const Keyword);

} // namespace ast
