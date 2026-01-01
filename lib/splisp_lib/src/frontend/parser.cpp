#include <charconv>
#include <cstdint>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <stdlib.h>
#include <utility>
#include <variant>

Parser::Parser(Lexer lex) : lex(std::move(lex)) {}

AST Parser::parse() {
  AST ast;
  while (this->lex.peek()) {
    ast.push_back(create_sexp());
  }
  return ast;
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

std::unique_ptr<SExp> Parser::create_sexp() {
  // function to construct a single s-exp
  Token next = lex.next().value();
  switch (next.kind) {
  case (TokenKind::lparn): {
    SExp sexp = {.node = create_list()};
    return std::make_unique<SExp>(std::move(sexp));
  }
  case (TokenKind::atoms): {
    SExp sexp;
    auto num = is_number(next.lexeme);
    if (num) {
      sexp = {.node = Symbol{num.value()}};
      return std::make_unique<SExp>(std::move(sexp));
    };
    auto truthy = is_bool(next.lexeme);
    if (truthy) {
      sexp = {.node = Symbol{truthy.value()}};
      return std::make_unique<SExp>(std::move(sexp));
    };
    auto kword = is_keyword(next.lexeme);
    if (kword) {
      sexp = {.node = Symbol{kword.value()}};
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
    SExp sexp = {.node = Symbol{next.lexeme}};
    return std::make_unique<SExp>(std::move(sexp));
  }
  }
  throw std::logic_error("strange construction");
}

List Parser::create_list() {
  List ret = List();
  while (lex.peek().value().kind != TokenKind::rparn) {
    ret.list.push_back((create_sexp()));
  }
  lex.next();
  return ret;
}
