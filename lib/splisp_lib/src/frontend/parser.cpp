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
#include <vector>

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
                case (ast::Keyword::letrec): {
                  sexp = create_letrec(p);
                  resolve_forms(sexp, false);
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
  auto &vars_list = std::get<ast::List>(vars.node);
  std::vector<std::unique_ptr<ast::SExp>> values_list;

  // construction of the vars and values list
  if (List *args = std::get_if<ast::List>(&list.list.at(1)->node)) {
    for (auto &arg : args->list) {
      if (List *pair = std::get_if<ast::List>(&arg->node)) {
        if (std::get_if<ast::Symbol>(&pair->list.at(0)->node)) {
          vars_list.list.push_back(std::move(pair->list.at(0)));
        }
        values_list.push_back(std::move(pair->list.at(1)));
      }
    }
  }

  List lambda_list;
  lambda_list.list.push_back(
      std::make_unique<SExp>(SExp{.node = Symbol{Keyword::lambda}}));
  lambda_list.list.push_back(std::make_unique<SExp>(std::move(vars)));
  for (size_t i = 2; i < list.list.size(); i++) {
    lambda_list.list.push_back(std::move(list.list.at(i)));
  }
  desugared.list.push_back(
      std::make_unique<SExp>(SExp{.node = std::move(lambda_list)}));
  for (auto &value : values_list) {
    desugared.list.push_back(std::move(value));
  }
  return SExp{.node = std::move(desugared)};
}

SExp Parser::create_letrec(List &list) {
  // letrec -> let + set -> lambda
  // (letrec (x_i e_i)* (body)) -> (let (x_i nil)* (set! x_i e_i)* body)
  // List(Symbol(letrec) List(List(Symbol(name) SExp(defintion))) SExp(Body))
  // ->
  // List(Symbol(let) List(List(List(Symbol(name) UNDEF)) +
  // List(List(Keyword(set!) Symbol(name) SExp(defintion)))) SExp(Body))
  List desugared;
  std::vector<std::unique_ptr<ast::SExp>> definitions_list; // SExp(definition)
  std::vector<ast::Symbol> names; // List(Symbol(name))
  List undefined_names;           // List(List(Symbol(name) Symbol(UNDEF)))
  List set_names; // List(Keyword(set!) Symbol(name) SExp(Defintion))

  // extract from the original list the names and definitions of each argument
  if (List *args = std::get_if<ast::List>(&list.list.at(1)->node)) {
    for (auto &arg : args->list) {
      // for every arg in the arguments, we disassemble into name + def tuples
      if (List *tuple = std::get_if<ast::List>(&arg->node)) {
        if (auto *sym = std::get_if<ast::Symbol>(&tuple->list.at(0)->node)) {
          names.push_back(*sym);
        }
        definitions_list.push_back(std::move(tuple->list.at(1)));
      }
    }
  }

  // build the undefined names List
  for (const auto &name : names) {
    List entry;
    entry.list.push_back(make_sexp(name));
    entry.list.push_back(make_sexp(Symbol{Undef{}}));
    undefined_names.list.push_back(make_sexp(std::move(entry)));
  }

  // build the defined names list
  for (size_t i = 0; i < definitions_list.size(); i++) {
    List entry;
    entry.list.push_back(make_sexp(Symbol{Keyword::set}));
    entry.list.push_back(make_sexp(names[i]));
    entry.list.push_back(std::move(definitions_list[i]));
    set_names.list.push_back(make_sexp(std::move(entry)));
  }

  // concatenate the the desugared list
  desugared.list.push_back(make_sexp(Symbol{Keyword::let}));
  desugared.list.push_back(make_sexp(std::move(undefined_names)));
  for (auto &set_name : set_names.list) {
    desugared.list.push_back(std::move(set_name));
  }
  for (size_t i = 2; i < list.list.size(); i++) {
    desugared.list.push_back(std::move(list.list.at(i)));
  }

  return create_let(desugared);
}

List Parser::create_list() {
  List ret = List();
  while (lex.peek().value().kind != TokenKind::rparn) {
    ret.list.push_back((create_sexp()));
  }
  lex.next();
  return ret;
}
