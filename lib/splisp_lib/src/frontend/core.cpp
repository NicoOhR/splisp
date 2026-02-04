#include "frontend/ast.hpp"
#include <boost/throw_exception.hpp>
#include <cstdint>
#include <cstdlib>
#include <frontend/core.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
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
        }
      }
    }
    return lower_expr(sexp);
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
        case (ast::Keyword::define): {
          ret.node = lower_definition(sexp);
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
core::Cond core::Lowerer::lower_condition(const ast::SExp &sexp) {
  // List(Keyword(If) SExp(cond) SExp(Then) SExp(Else))
  core::Cond ret;
  if (const auto lst = std::get_if<ast::List>(&sexp.node)) {
    ret.condition = std::make_unique<core::Expr>(
        core::Lowerer::lower_expr(*lst->list.at(1)));
    ret.then = std::make_unique<core::Expr>(
        core::Lowerer::lower_expr(*lst->list.at(2)));
    ret.otherwise = std::make_unique<core::Expr>(
        core::Lowerer::lower_expr(*lst->list.at(3)));
  }
  return ret;
}

namespace core {

namespace {
void print_indent(int level) { std::cout << std::string(level * 2, ' '); }
} // namespace

void print_const(const Const &konst, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Const " << konst.value;
}

void print_var(const Var &var, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Var " << var.id;
}

void print_apply(const Apply &apply, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Apply";

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Callee";
  if (apply.callee) {
    print_expr(*apply.callee, level + 2);
  } else {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<null>";
  }

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Args";
  if (apply.args.empty()) {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<empty>";
    return;
  }

  for (const auto &arg : apply.args) {
    if (arg) {
      print_expr(*arg, level + 2);
    } else {
      std::cout << std::endl;
      print_indent(level + 2);
      std::cout << "<null>";
    }
  }
}

void print_lambda(const Lambda &lambda, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Lambda";

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Formals";
  if (lambda.formals.empty()) {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<empty>";
  } else {
    for (const auto &formal : lambda.formals) {
      std::cout << std::endl;
      print_indent(level + 2);
      if (formal) {
        std::cout << "SymbolId " << *formal;
      } else {
        std::cout << "<null>";
      }
    }
  }

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Body";
  if (lambda.body.empty()) {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<empty>";
    return;
  }
  for (const auto &expr : lambda.body) {
    if (expr) {
      print_expr(*expr, level + 2);
    } else {
      std::cout << std::endl;
      print_indent(level + 2);
      std::cout << "<null>";
    }
  }
}

void print_cond(const Cond &cond, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Cond";

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Condition";
  if (cond.condition) {
    print_expr(*cond.condition, level + 2);
  } else {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<null>";
  }

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Then";
  if (cond.then) {
    print_expr(*cond.then, level + 2);
  } else {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<null>";
  }

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Otherwise";
  if (cond.otherwise) {
    print_expr(*cond.otherwise, level + 2);
  } else {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<null>";
  }
}

void print_define(const Define &defn, int level) {
  std::cout << std::endl;
  print_indent(level);
  std::cout << "Define";

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Name " << defn.name;

  std::cout << std::endl;
  print_indent(level + 1);
  std::cout << "Rhs";
  if (defn.rhs) {
    print_expr(*defn.rhs, level + 2);
  } else {
    std::cout << std::endl;
    print_indent(level + 2);
    std::cout << "<null>";
  }
}

void print_expr(const Expr &expr, int level) {
  std::visit(
      [level](const auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Apply>) {
          print_apply(node, level);
        } else if constexpr (std::is_same_v<T, Lambda>) {
          print_lambda(node, level);
        } else if constexpr (std::is_same_v<T, Const>) {
          print_const(node, level);
        } else if constexpr (std::is_same_v<T, Cond>) {
          print_cond(node, level);
        } else if constexpr (std::is_same_v<T, Var>) {
          print_var(node, level);
        } else if constexpr (std::is_same_v<T, Define>) {
          print_define(node, level);
        }
      },
      expr.node);
}

void print_top(const Top &top, int level) {
  std::visit(
      [level](const auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Define>) {
          print_define(node, level);
        } else if constexpr (std::is_same_v<T, Expr>) {
          print_expr(node, level);
        }
      },
      top);
}

void print_program(const Program &program) {
  std::cout << "Core";
  for (const auto &top : program) {
    print_top(top, 1);
  }
}

} // namespace core
