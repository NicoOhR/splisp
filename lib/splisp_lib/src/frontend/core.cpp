#include "frontend/ast.hpp"
#include <boost/throw_exception.hpp>
#include <cstdint>
#include <cstdlib>
#include <frontend/core.hpp>
#include <memory>
#include <stdexcept>
#include <variant>

core::Program &core::Lowerer::lower(const ast::AST &ast) {
  program_.clear();
  for (auto &sexp : ast) {
    program_.push_back(lower_top(*sexp));
  }
  return program_;
}

core::Top core::Lowerer::lower_top(const ast::SExp &sexp) {
  // match special forms, if not special form then Apply
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    if (const auto sym = std::get_if<ast::Symbol>(&lst->list.front()->node)) {
      if (const auto kw = std::get_if<ast::Keyword>(&sym->value)) {
        if (*kw == ast::Keyword::define) {
          return lower_definition(sexp);
        } else {
          return lower_expr(sexp);
        }
      }
    }
  }
  throw std::invalid_argument("unhandled SExp in lower_top");
}

// this can accept a const List to avoid the top level stripping
core::Define core::Lowerer::lower_definition(const ast::SExp &sexp) {
  // List(Keyword(Define) SymbolId SExp)
  core::Define ret;
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    if (const auto sym = std::get_if<ast::Symbol>(&lst->list.at(1)->node)) {
      if (const auto sym_id = std::get_if<ast::SymbolID>(&sym->value)) {
        ret.name = sym_id->id;
      }
    }
    core::Expr body = lower_expr(*lst->list.at(2));
    ret.rhs = std::make_unique<core::Expr>(std::move(body));
  }
  return ret;
}

core::Expr core::Lowerer::lower_expr(const ast::SExp &sexp) {
  core::Expr ret;
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    if (const auto sym = std::get_if<ast::Symbol>(&lst->list.at(0)->node)) {
      if (const auto kw = std::get_if<ast::Keyword>(&sym->value)) {
        switch (*kw) {
        case (ast::Keyword::if_expr): {
          ret.node = lower_condition(sexp);
          return ret;
        }
        case (ast::Keyword::lambda): {
          ret.node = lower_lambda(sexp);
          return ret;
        }
        default:
          break;
        }
      }
    }
    ret.node = lower_apply(sexp);
    return ret;
  }
  if (const auto sym = std::get_if<ast::Symbol>(&sexp.node)) {
    if (std::get_if<bool>(&sym->value) ||
        std::get_if<std::uint64_t>(&sym->value)) {
      ret.node = lower_const(sexp);
    } else {
      ret.node = lower_var(sexp);
    }
  }
  return ret;
}

core::Apply core::Lowerer::lower_apply(const ast::SExp &sexp) {
  core::Apply ret;
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    auto callee = std::make_unique<core::Expr>(
        core::Lowerer::lower_expr(*lst->list.at(0)));
    ret.callee = std::move(callee);
    if (lst->list.size() > 1) {
      for (size_t i = 1; i < lst->list.size(); i++) {
        ret.args.push_back(std::make_unique<core::Expr>(
            core::Lowerer::lower_expr(*lst->list.at(i))));
      }
    }
    return ret;
  }
  throw std::invalid_argument("Invalid application parsed");
}

core::Const core::Lowerer::lower_const(const ast::SExp &sexp) {
  if (const auto sym = std::get_if<ast::Symbol>(&sexp.node)) {
    if (const auto val = std::get_if<uint64_t>(&sym->value)) {
      return Const{*val};
    }
    if (const auto val = std::get_if<bool>(&sym->value)) {
      return val ? Const{1} : Const{0};
    }
  }
}

core::Var core::Lowerer::lower_var(const ast::SExp &sexp) {
  core::Var ret;
  if (const auto sym = std::get_if<ast::Symbol>(&sexp.node)) {
    if (const auto id = std::get_if<ast::SymbolID>(&sym->value)) {
      ret.id = id->id;
    }
  }
  return ret;
}

core::Lambda core::Lowerer::lower_lambda(const ast::SExp &sexp) {
  // List(Kword(Lambda) List(args) List(Body) List(Body)*)
  core::Lambda ret;
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    if (const auto args = std::get_if<ast::List>(&lst->list.at(1)->node)) {
      for (size_t i = 0; i < args->list.size(); i++) {
        if (const auto sym =
                std::get_if<ast::Symbol>(&args->list.at(i)->node)) {
          if (const auto id = std::get_if<ast::SymbolID>(&sym->value)) {
            ret.formals.push_back(std::make_unique<core::SymbolId>(id->id));
          }
        }
      }
      for (size_t i = 2; i < lst->list.size(); i++) {
        ret.body.push_back(std::make_unique<core::Expr>(
            core::Lowerer::lower_expr(*lst->list.at(i))));
      }
    }
  }
  return ret;
}
core::Cond lower_condition(const ast::SExp &sexp) {
  // List(Keyword(If) SExp(cond) SExp(Then) SExp(Else))
}
