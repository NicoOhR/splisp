#include "frontend/ast.hpp"
#include <charconv>
#include <cstdint>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <stdlib.h>
#include <type_traits>
#include <utility>
#include <variant>

Parser::Parser(Lexer lex) : lex(std::move(lex)) {}

AST Parser::parse() {
  AST ast;
  while (this->lex.peek()) {
    ast.push_back(create_sexp());
  }
  for (auto &sexp : ast) {
    resolve_forms(*sexp, true);
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
    sexp = {.node = Symbol{next.lexeme}};
    return std::make_unique<SExp>(std::move(sexp));
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

void Parser::resolve_forms(SExp &sexp, bool is_top_level) {
  std::visit(
      [this, &sexp, is_top_level](auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ast::Symbol>) {
          return;
        }
        if constexpr (std::is_same_v<T, List>) {
          if (!p.list.empty()) {
            if (auto *sym = std::get_if<ast::Symbol>(&p.list.front()->node)) {
              if (auto *kw = std::get_if<ast::Keyword>(&sym->value)) {
                switch (*kw) {
                case (ast::Keyword::define): {
                  if (!is_top_level) {
                    throw std::invalid_argument(
                        "Define is only allowed at the top level");
                  }
                  sexp.node = create_define(p);
                  if (auto *list = std::get_if<ast::List>(&sexp.node)) {
                    for (auto &elem : list->list) {
                      resolve_forms(*elem, false);
                    }
                  }
                  return;
                }
                case (ast::Keyword::lambda): {
                  sexp.node = create_lambda(p);
                  if (auto *list = std::get_if<ast::List>(&sexp.node)) {
                    for (auto &elem : list->list) {
                      resolve_forms(*elem, false);
                    }
                  }
                  return;
                }
                case (ast::Keyword::let): {
                  sexp = create_let(p);
                  resolve_forms(sexp, false);
                  return;
                }
                default: {
                  break;
                }
                }
              }
            }
          }
          for (auto &elem : p.list) {
            resolve_forms(*elem, false);
          }
        }
      },
      sexp.node);
  return;
}

ast::List Parser::create_lambda(List &list) {
  //(lambda (args) (sexp) (sexp) ... )
  if (list.list.size() < 3) {
    throw std::invalid_argument("Lambda requires args and body");
  }
  if (!std::get_if<ast::List>(&list.list.at(1)->node)) {
    throw std::invalid_argument("Args Should be a List");
  }
  List ret;
  for (size_t i = 0; i < list.list.size(); i++) {
    ret.list.push_back(std::move(list.list.at(i)));
  }
  return ret;
}

List Parser::create_define(List &list) {
  // supported forms:
  // (define name body)
  // (define (name args...) body) -> (define name (lambda (args...) body))
  if (list.list.size() < 3) {
    throw std::invalid_argument("Define requires a name and body");
  }

  std::unique_ptr<SExp> name;
  std::unique_ptr<SExp> body;

  if (auto *signature = std::get_if<ast::List>(&list.list.at(1)->node)) {
    if (signature->list.empty()) {
      throw std::invalid_argument("Invalid function name");
    }
    name = std::move(signature->list.at(0));
    if (!ast::to_string(*name)) {
      throw std::invalid_argument("Invalid function name");
    }
    SExp args_sexp = {.node = List()};
    auto &args_list = std::get<ast::List>(args_sexp.node);
    for (size_t i = 1; i < signature->list.size(); ++i) {
      args_list.list.push_back(std::move(signature->list.at(i)));
    }
    auto args = std::make_unique<SExp>(std::move(args_sexp));
    body = std::move(list.list.at(2));
    if (!std::get_if<ast::List>(&args->node)) {
      throw std::invalid_argument("Args Should be a List");
    }

    List lambda_list;
    lambda_list.list.push_back(
        std::make_unique<SExp>(SExp{.node = Symbol{Keyword::lambda}}));
    lambda_list.list.push_back(std::move(args));
    lambda_list.list.push_back(std::move(body));

    List ret;
    ret.list.push_back(std::move(list.list.at(0)));
    ret.list.push_back(std::move(name));
    ret.list.push_back(
        std::make_unique<SExp>(SExp{.node = std::move(lambda_list)}));
    return ret;
  }

  name = std::move(list.list.at(1));
  if (!ast::to_string(*name)) {
    throw std::invalid_argument("Invalid function name");
  }
  if (list.list.size() != 3) {
    throw std::invalid_argument(
        "Function definitions must use (define (name args...) body)");
  }

  body = std::move(list.list.at(2));
  List ret;
  ret.list.push_back(std::move(list.list.at(0)));
  ret.list.push_back(std::move(name));
  ret.list.push_back(std::move(body));
  return ret;
}

SExp Parser::create_let(List &list) {
  //(let ((x e1) (x e2)) (body)) <-> (lambda (x y) (body) (e1 e2))
  // assumed form: List(Symbol(let) List(List(args)) SExp(Body))
  // result after parsing List(List(lambda List(Args) SExp(body)) List(values))
  List desugared;
  SExp vars = {.node = List()};
  SExp values = {.node = List()};
  auto &vars_list = std::get<ast::List>(vars.node);
  auto &values_list = std::get<ast::List>(values.node);

  // construction of the vars and values list
  if (List *args = std::get_if<ast::List>(&list.list.at(1)->node)) {
    for (auto &arg : args->list) {
      if (List *pair = std::get_if<ast::List>(&arg->node)) {
        if (std::get_if<ast::Symbol>(&pair->list.at(0)->node)) {
          vars_list.list.push_back(std::move(pair->list.at(0)));
        }
        if (std::get_if<ast::Symbol>(&pair->list.at(1)->node)) {
          values_list.list.push_back(std::move(pair->list.at(1)));
        }
      }
    }
  }

  List lambda_list;
  lambda_list.list.push_back(
      std::make_unique<SExp>(SExp{.node = Symbol{Keyword::lambda}}));
  lambda_list.list.push_back(std::make_unique<SExp>(std::move(vars)));
  lambda_list.list.push_back(std::move(list.list.at(2)));
  desugared.list.push_back(
      std::make_unique<SExp>(SExp{.node = std::move(lambda_list)}));
  desugared.list.push_back(std::make_unique<SExp>(std::move(values)));
  return SExp{.node = std::move(desugared)};
}

List Parser::create_list() {
  List ret = List();
  while (lex.peek().value().kind != TokenKind::rparn) {
    ret.list.push_back((create_sexp()));
  }
  lex.next();
  return ret;
}
