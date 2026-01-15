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
  std::vector<std::unique_ptr<SymbolId>> formals;
  std::vector<std::unique_ptr<Expr>> body;
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
  std::variant<Apply, Lambda, Const, Cond, Var> node;
};

using Top = std::variant<Define, Expr>;

using Program = std::vector<Top>;

void print_const(const Const &konst, int level);
void print_var(const Var &var, int level);
void print_apply(const Apply &apply, int level);
void print_lambda(const Lambda &lambda, int level);
void print_cond(const Cond &cond, int level);
void print_define(const Define &defn, int level);
void print_expr(const Expr &expr, int level);
void print_top(const Top &top, int level);
void print_program(const Program &program);

class Lowerer {
public:
  Program &lower(const ast::AST &ast);

private:
  Top lower_top(const ast::SExp &sexp);           //
  Expr lower_expr(const ast::SExp &sexp);         //
  Define lower_definition(const ast::SExp &sexp); //

  Const lower_const(const ast::SExp &sexp); //
  Var lower_var(const ast::SExp &sexp);
  Apply lower_apply(const ast::SExp &sexp); //
  Lambda lower_lambda(const ast::SExp &sexp);
  Cond lower_condition(const ast::SExp &sexp);

  Program program_;
};

} // namespace core
