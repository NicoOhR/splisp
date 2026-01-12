#pragma once

#include "frontend/ast.hpp"
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>
namespace core {
struct Expr;

using SymbolId = std::uint64_t;

struct Const {
  uint64_t value;
};

struct Var {
  SymbolId id;
};

struct Apply {
  std::unique_ptr<Expr> callee;
  std::vector<std::unique_ptr<Expr>> args;
};

struct Lambda {
  std::vector<std::unique_ptr<SymbolId>> params;
  std::unique_ptr<Expr> body;
};

struct Cond {
  std::unique_ptr<Expr> condition;
  std::unique_ptr<Expr> then;
  std::unique_ptr<Expr> otherwise;
};

struct Define {
  // global scope definitions
  SymbolId name;
  std::unique_ptr<Expr> rhs;
};

struct Expr {
  std::variant<Apply, Lambda, Const, Cond> node;
};

using Top = std::variant<Define, Expr>;

using Program = std::vector<Top>;

Program from_ast(ast::AST);

} // namespace core
