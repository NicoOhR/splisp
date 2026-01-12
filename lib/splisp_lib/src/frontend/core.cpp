#include <frontend/core.hpp>
#include <type_traits>
#include <variant>

core::Program core::from_ast(const ast::AST &ast) {
  std::vector<core::Top> ret;
  for (auto &sexp : ast) {
    ret.push_back(lower_top(*sexp));
  }
  return ret;
}

core::Top core::lower_top(const ast::SExp &sexp) {}
