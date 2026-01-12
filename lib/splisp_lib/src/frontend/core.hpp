#include <frontend/core.hpp>
#include <type_traits>
#include <variant>

core::Program core::from_ast(ast::AST &ast) {
  for (auto &sexp : ast) {
    // SExps need to be identified as a top level expression
    std::visit(
        [](auto &p) {
          using T = std::decay_t<decltype(p)>;
          if constexpr (std::is_same_v<T, ast::Symbol>) {
          }
          if constexpr (std::is_same_v<T, ast::List>) {
          }
        },
        sexp->node);
  }
}
