#include <ast.hpp>
#include <iostream>

namespace ast {

void print_symbol(const Symbol &sym, int level) {
  std::visit(
      [level](const auto &v) {
        using T = std::decay_t<decltype(v)>;
        std::string stuff(level * 2, ' ');
        if constexpr (std::is_same_v<T, std::string>) {
          std::cout << stuff << "Ident " << v;
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
          std::cout << stuff << "Int " << v;
        } else if constexpr (std::is_same_v<T, bool>) {
          std::cout << stuff << "Bool " << (v ? "true" : "false");
        } else if constexpr (std::is_same_v<T, Keyword>) {
          std::cout << stuff << "Kword " << print_keyword(v);
        }
      },
      sym.value);
}

void print_sexp(const SExp &sexp, int level) {
  std::cout << std::endl;
  std::string stuff(level * 2, ' ');
  if (auto *lst = std::get_if<List>(&sexp.node)) {
    std::cout << stuff << "List";
    for (const auto &ptr : lst->list) {
      if (ptr) {
        ast::print_sexp(*ptr, level + 1);
      }
    }
  } else if (auto *sym = std::get_if<Symbol>(&sexp.node)) {
    ast::print_symbol(*sym, level);
  }
}

std::string print_keyword(const Keyword kword) {
  switch (kword) {
  case (Keyword::if_expr): {
    return "if";
  }
  case (Keyword::define): {
    return "define";
  }
  case (Keyword::lambda): {
    return "lambda";
  }
  }
  throw std::invalid_argument("Keyword not recogonized");
}

void print_ast(const AST &ast) {
  std::cout << "AST";
  for (const auto &root : ast) {
    ast::print_sexp(*root, 1);
  }
}

} // namespace ast
