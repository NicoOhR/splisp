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
#include <type_traits>
#include <utility>
#include <variant>

void Parser::print_symbol(const Symbol &sym, int level) const {
  std::visit(
      [level, this](const auto &v) {
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

void Parser::print_sexp(const SExp &sexp, int level) const {
  std::cout << std::endl;
  std::string stuff(level * 2, ' ');
  if (auto *lst = std::get_if<List>(&sexp.node)) {
    std::cout << stuff << "List";
    for (const auto &ptr : lst->list) {
      if (ptr) {
        print_sexp(*ptr, level + 1);
      }
    }
  } else if (auto *sym = std::get_if<Symbol>(&sexp.node)) {
    print_symbol(*sym, level);
  }
}

std::string Parser::print_keyword(const Keyword kword) const {
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

void Parser::print_ast() const {
  std::cout << "AST";
  for (const auto &root : AST) {
    print_sexp(*root, 1);
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
