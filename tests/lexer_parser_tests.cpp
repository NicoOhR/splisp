#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

#include <gtest/gtest.h>

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>

namespace {

const ast::List *as_list(const ast::SExp &sexp) {
  return std::get_if<ast::List>(&sexp.node);
}

const ast::Symbol *as_symbol(const ast::SExp &sexp) {
  return std::get_if<ast::Symbol>(&sexp.node);
}

const ast::SExp *list_item(const ast::List &list, std::size_t index) {
  if (index >= list.list.size() || list.list[index] == nullptr) {
    return nullptr;
  }
  return list.list[index].get();
}

TEST(LexerTests, BasicTokens) {
  Lexer lex("(+ 1 #t)");

  auto peeked = lex.peek();
  ASSERT_TRUE(peeked.has_value());
  EXPECT_EQ(peeked->kind, TokenKind::lparn);
  EXPECT_EQ(peeked->lexeme, "(");

  auto first = lex.next();
  ASSERT_TRUE(first.has_value());
  EXPECT_EQ(first->kind, TokenKind::lparn);
  EXPECT_EQ(first->lexeme, "(");

  auto second = lex.next();
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(second->kind, TokenKind::ident);
  EXPECT_EQ(second->lexeme, "+");

  auto third = lex.next();
  ASSERT_TRUE(third.has_value());
  EXPECT_EQ(third->kind, TokenKind::atoms);
  EXPECT_EQ(third->lexeme, "1");

  auto fourth = lex.next();
  ASSERT_TRUE(fourth.has_value());
  EXPECT_EQ(fourth->kind, TokenKind::atoms);
  EXPECT_EQ(fourth->lexeme, "#t");

  auto fifth = lex.next();
  ASSERT_TRUE(fifth.has_value());
  EXPECT_EQ(fifth->kind, TokenKind::rparn);
  EXPECT_EQ(fifth->lexeme, ")");

  EXPECT_FALSE(lex.next().has_value());
}

TEST(ParserTests, SimpleList) {
  Parser parser(Lexer("(+ 1 2)"));
  ast::AST ast = parser.parse();

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *list = as_list(*ast[0]);
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->list.size(), 3U);

  const auto *op_item = list_item(*list, 0);
  ASSERT_NE(op_item, nullptr);
  const auto *op = as_symbol(*op_item);
  ASSERT_NE(op, nullptr);
  const auto *op_name = std::get_if<std::string>(&op->value);
  ASSERT_NE(op_name, nullptr);
  EXPECT_EQ(*op_name, "+");

  const auto *lhs_item = list_item(*list, 1);
  ASSERT_NE(lhs_item, nullptr);
  const auto *lhs = as_symbol(*lhs_item);
  ASSERT_NE(lhs, nullptr);
  const auto *lhs_val = std::get_if<std::uint64_t>(&lhs->value);
  ASSERT_NE(lhs_val, nullptr);
  EXPECT_EQ(*lhs_val, 1U);

  const auto *rhs_item = list_item(*list, 2);
  ASSERT_NE(rhs_item, nullptr);
  const auto *rhs = as_symbol(*rhs_item);
  ASSERT_NE(rhs, nullptr);
  const auto *rhs_val = std::get_if<std::uint64_t>(&rhs->value);
  ASSERT_NE(rhs_val, nullptr);
  EXPECT_EQ(*rhs_val, 2U);
}

TEST(ParserTests, KeywordsAndBools) {
  Parser parser(Lexer("(if #t 1 0)"));
  ast::AST ast = parser.parse();

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *list = as_list(*ast[0]);
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->list.size(), 4U);

  const auto *kw_item = list_item(*list, 0);
  ASSERT_NE(kw_item, nullptr);
  const auto *kw = as_symbol(*kw_item);
  ASSERT_NE(kw, nullptr);
  const auto *kw_val = std::get_if<ast::Keyword>(&kw->value);
  ASSERT_NE(kw_val, nullptr);
  EXPECT_EQ(*kw_val, ast::Keyword::if_expr);

  const auto *cond_item = list_item(*list, 1);
  ASSERT_NE(cond_item, nullptr);
  const auto *cond = as_symbol(*cond_item);
  ASSERT_NE(cond, nullptr);
  const auto *cond_val = std::get_if<bool>(&cond->value);
  ASSERT_NE(cond_val, nullptr);
  EXPECT_TRUE(*cond_val);

  const auto *then_item = list_item(*list, 2);
  ASSERT_NE(then_item, nullptr);
  const auto *then_val = as_symbol(*then_item);
  ASSERT_NE(then_val, nullptr);
  const auto *then_num = std::get_if<std::uint64_t>(&then_val->value);
  ASSERT_NE(then_num, nullptr);
  EXPECT_EQ(*then_num, 1U);

  const auto *else_item = list_item(*list, 3);
  ASSERT_NE(else_item, nullptr);
  const auto *else_val = as_symbol(*else_item);
  ASSERT_NE(else_val, nullptr);
  const auto *else_num = std::get_if<std::uint64_t>(&else_val->value);
  ASSERT_NE(else_num, nullptr);
  EXPECT_EQ(*else_num, 0U);
}

TEST(ParserTests, MismatchedParensThrows) {
  Parser parser(Lexer(")"));
  EXPECT_THROW((void)parser.parse(), std::logic_error);
}

TEST(ParserTests, InvalidAtomThrows) {
  Parser parser(Lexer("foo"));
  EXPECT_THROW((void)parser.parse(), std::invalid_argument);
}

} // namespace
