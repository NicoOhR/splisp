#include "lexer.hpp"
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <parser.hpp>
#include <stdexcept>
#include <utility>
#include <variant>

void Parser::print_sexp(const SExp &sexp) const {
  if (auto *lst = std::get_if<List>(&sexp.node)) {
    std::cout << std::endl;
    for (const auto &ptr : lst->list) {
      if (ptr) {
        print_sexp(*ptr);
      }
    }
  } else if (auto *sym = std::get_if<Symbol>(&sexp.node)) {
    std::visit(
        [](const auto &v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, std::string>) {
            std::cout << " " << v;
          } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            std::cout << " " << v;
          } else if constexpr (std::is_same_v<T, bool>) {
            std::cout << (v ? " true " : " false ");
          }
        },
        sym->value);
  }
}

void Parser::print_ast() const {
  for (const auto &root : AST) {
    print_sexp(*root);
  }
}

Parser::Parser(Lexer &lex) {
  while (lex.peek()) {
    AST.push_back(create_sexp(lex));
  }
  return;
}

std::unique_ptr<SExp> Parser::create_sexp(Lexer &lex) {
  // function to construct a single s-exp
  switch (lex.next().value().kind) {
  case (TokenKind::lparn): {
    SExp sexp = {.node = create_list(lex)};
    return std::make_unique<SExp>(std::move(sexp));
  }
  case (TokenKind::atoms): {
    SExp sexp = {.node = Symbol("thing")};
    return std::make_unique<SExp>(std::move(sexp));
  }
  case (TokenKind::rparn): {
    throw std::logic_error("mismatched parantheses");
    break;
  }
  case (TokenKind::ident): {
    SExp sexp = {.node = Symbol("operation")};
    return std::make_unique<SExp>(std::move(sexp));
  }
  }
}

List Parser::create_list(Lexer &lex) {
  List ret = List();
  while (lex.peek().value().kind != TokenKind::rparn) {
    ret.list.push_back((create_sexp(lex)));
  }
  lex.next();
  return ret;
}
