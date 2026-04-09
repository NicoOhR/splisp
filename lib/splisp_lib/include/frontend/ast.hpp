#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ast {
enum Keyword { if_expr, let, letrec, lambda, define, set };

struct SExp;

// this needs to be shared with core
struct SymbolID {
  uint64_t id;
};

struct Undef {};

struct Symbol {
  std::variant<Keyword, std::string, SymbolID, std::uint64_t, bool, Undef>
      value;
};

struct List {
  std::vector<std::unique_ptr<SExp>> list;
};

struct SExp {
  std::variant<List, Symbol> node;
  std::optional<size_t>
      scope_id; // need to switch to an initalizer to make this a nullopt
};

using AST = std::vector<std::unique_ptr<SExp>>;

template <typename T> std::unique_ptr<SExp> make_sexp(T node) {
  return std::make_unique<SExp>(SExp{.node = std::move(node)});
}

std::optional<std::string> to_string(const SExp &sexp);
void print_ast(const AST &ast);
void print_sexp(const SExp &sexp, int level);
void print_symbol(const Symbol &sym, int level);
std::string print_keyword(const Keyword);

} // namespace ast
