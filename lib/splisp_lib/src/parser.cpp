#include "lexer.hpp"
#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <parser.hpp>
#include <stdexcept>
#include <stdlib.h>
#include <utility>
#include <variant>

void Parser::print_symbol(const Symbol &sym) const {
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
      sym.value);
}

void Parser::print_sexp(const SExp &sexp) const {
  if (auto *lst = std::get_if<List>(&sexp.node)) {
    for (const auto &ptr : lst->list) {
      if (ptr) {
        print_sexp(*ptr);
      }
    }
  } else if (auto *sym = std::get_if<Symbol>(&sexp.node)) {
    print_symbol(*sym);
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

std::optional<uint64_t> Parser::is_number(std::string str) {
  int i{};
  std::from_chars_result res =
      std::from_chars(str.data(), str.data() + str.size(), i);
  if (res.ec != std::errc{} or res.ptr != str.data() + str.size()) {
    return std::nullopt;
  }
  return i;
}

std::optional<bool> Parser::is_bool(std::string str) {
  if (str == "#t") {
    return true;
  } else if (str == "#f") {
    return false;
  } else {
    return std::nullopt;
  }
}

std::optional<Keyword> Parser::is_keyword(std::string str) {
  auto match = kwords.find(str);
  if (match != kwords.end()) {
    return match->second;
  } else {
    return std::nullopt;
  }
}

std::unique_ptr<SExp> Parser::create_sexp(Lexer &lex) {
  // function to construct a single s-exp
  Token next = lex.next().value();
  switch (next.kind) {
  case (TokenKind::lparn): {
    SExp sexp = {.node = create_list(lex)};
    return std::make_unique<SExp>(std::move(sexp));
  }
  case (TokenKind::atoms): {
    SExp sexp;
    auto num = is_number(next.lexeme);
    if (num) {
      sexp = {.node = Symbol(num.value())};
      return std::make_unique<SExp>(std::move(sexp));
    };
    auto truthy = is_bool(next.lexeme);
    if (truthy) {
      sexp = {.node = Symbol(truthy.value())};
      return std::make_unique<SExp>(std::move(sexp));
    };
    auto kword = is_keyword(next.lexeme);
    if (kword) {
      sexp = {.node = Symbol(kword.value())};
      return std::make_unique<SExp>(std::move(sexp));
    }
    throw std::invalid_argument("invalid atom");
    break;
  }
  case (TokenKind::rparn): {
    throw std::logic_error("mismatched parantheses");
    break;
  }
  case (TokenKind::ident): {
    SExp sexp = {.node = Symbol(next.lexeme)};
    return std::make_unique<SExp>(std::move(sexp));
  }
  }
  throw std::logic_error("strange construction");
}

List Parser::create_list(Lexer &lex) {
  List ret = List();
  while (lex.peek().value().kind != TokenKind::rparn) {
    ret.list.push_back((create_sexp(lex)));
  }
  lex.next();
  return ret;
}
