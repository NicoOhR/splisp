#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>

namespace {

const ast::List &require_list(const ast::SExp &sexp) {
  const auto *lst = std::get_if<ast::List>(&sexp.node);
  assert(lst != nullptr);
  return *lst;
}

const ast::Symbol &require_symbol(const ast::SExp &sexp) {
  const auto *sym = std::get_if<ast::Symbol>(&sexp.node);
  assert(sym != nullptr);
  return *sym;
}

const ast::SExp &require_list_item(const ast::List &list, std::size_t index) {
  assert(index < list.list.size());
  assert(list.list[index] != nullptr);
  return *list.list[index];
}

void test_lexer_basic_tokens() {
  Lexer lex("(+ 1 #t)");

  auto peeked = lex.peek();
  assert(peeked.has_value());
  assert(peeked->kind == TokenKind::lparn);
  assert(peeked->lexeme == "(");

  auto first = lex.next();
  assert(first.has_value());
  assert(first->kind == TokenKind::lparn);
  assert(first->lexeme == "(");

  auto second = lex.next();
  assert(second.has_value());
  assert(second->kind == TokenKind::ident);
  assert(second->lexeme == "+");

  auto third = lex.next();
  assert(third.has_value());
  assert(third->kind == TokenKind::atoms);
  assert(third->lexeme == "1");

  auto fourth = lex.next();
  assert(fourth.has_value());
  assert(fourth->kind == TokenKind::atoms);
  assert(fourth->lexeme == "#t");

  auto fifth = lex.next();
  assert(fifth.has_value());
  assert(fifth->kind == TokenKind::rparn);
  assert(fifth->lexeme == ")");

  assert(!lex.next().has_value());
}

void test_parser_simple_list() {
  Parser parser(Lexer("(+ 1 2)"));
  ast::AST ast = parser.parse();

  assert(ast.size() == 1);
  const auto &root = *ast[0];
  const auto &list = require_list(root);
  assert(list.list.size() == 3);

  const auto &op = require_symbol(require_list_item(list, 0));
  const auto *op_name = std::get_if<std::string>(&op.value);
  assert(op_name != nullptr);
  assert(*op_name == "+");

  const auto &lhs = require_symbol(require_list_item(list, 1));
  const auto *lhs_val = std::get_if<std::uint64_t>(&lhs.value);
  assert(lhs_val != nullptr);
  assert(*lhs_val == 1);

  const auto &rhs = require_symbol(require_list_item(list, 2));
  const auto *rhs_val = std::get_if<std::uint64_t>(&rhs.value);
  assert(rhs_val != nullptr);
  assert(*rhs_val == 2);
}

void test_parser_keywords_and_bools() {
  Parser parser(Lexer("(if #t 1 0)"));
  ast::AST ast = parser.parse();

  assert(ast.size() == 1);
  const auto &list = require_list(*ast[0]);
  assert(list.list.size() == 4);

  const auto &kw = require_symbol(require_list_item(list, 0));
  const auto *kw_val = std::get_if<ast::Keyword>(&kw.value);
  assert(kw_val != nullptr);
  assert(*kw_val == ast::Keyword::if_expr);

  const auto &cond = require_symbol(require_list_item(list, 1));
  const auto *cond_val = std::get_if<bool>(&cond.value);
  assert(cond_val != nullptr);
  assert(*cond_val == true);

  const auto &then_val = require_symbol(require_list_item(list, 2));
  const auto *then_num = std::get_if<std::uint64_t>(&then_val.value);
  assert(then_num != nullptr);
  assert(*then_num == 1);

  const auto &else_val = require_symbol(require_list_item(list, 3));
  const auto *else_num = std::get_if<std::uint64_t>(&else_val.value);
  assert(else_num != nullptr);
  assert(*else_num == 0);
}

void test_parser_mismatched_parens() {
  bool threw = false;
  try {
    Parser parser(Lexer(")"));
    (void)parser.parse();
  } catch (const std::logic_error &) {
    threw = true;
  }
  assert(threw);
}

void test_parser_invalid_atom() {
  bool threw = false;
  try {
    Parser parser(Lexer("foo"));
    (void)parser.parse();
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);
}

} // namespace

int main() {
  test_lexer_basic_tokens();
  test_parser_simple_list();
  test_parser_keywords_and_bools();
  test_parser_mismatched_parens();
  test_parser_invalid_atom();
  return 0;
}
