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

const ast::Keyword *as_keyword(const ast::SExp &sexp) {
  const auto *sym = as_symbol(sexp);
  if (!sym) {
    return nullptr;
  }
  return std::get_if<ast::Keyword>(&sym->value);
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
  EXPECT_EQ(second->kind, TokenKind::atoms);
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

TEST(ParserTests, UnknownAtomBecomesSymbol) {
  Parser parser(Lexer("foo"));
  ast::AST ast = parser.parse();

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *sym = as_symbol(*ast[0]);
  ASSERT_NE(sym, nullptr);
  const auto *sym_name = std::get_if<std::string>(&sym->value);
  ASSERT_NE(sym_name, nullptr);
  EXPECT_EQ(*sym_name, "foo");
}

TEST(ParserTests, DefineCreatesFunctionNode) {
  Parser parser(Lexer("(define (add x y) (+ x y))"));
  ast::AST ast = parser.parse();

  ast::print_ast(ast);
  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);

  const auto *define_list = as_list(*ast[0]);
  ASSERT_NE(define_list, nullptr);
  ASSERT_EQ(define_list->list.size(), 3U);

  const auto *define_kw_item = list_item(*define_list, 0);
  ASSERT_NE(define_kw_item, nullptr);
  const auto *define_kw = as_keyword(*define_kw_item);
  ASSERT_NE(define_kw, nullptr);
  EXPECT_EQ(*define_kw, ast::Keyword::define);

  const auto *name_item = list_item(*define_list, 1);
  ASSERT_NE(name_item, nullptr);
  const auto *name_sym = as_symbol(*name_item);
  ASSERT_NE(name_sym, nullptr);
  const auto *name_val = std::get_if<std::string>(&name_sym->value);
  ASSERT_NE(name_val, nullptr);
  EXPECT_EQ(*name_val, "add");

  const auto *lambda_item = list_item(*define_list, 2);
  ASSERT_NE(lambda_item, nullptr);
  const auto *lambda_list = as_list(*lambda_item);
  ASSERT_NE(lambda_list, nullptr);
  ASSERT_EQ(lambda_list->list.size(), 3U);

  const auto *lambda_kw_item = list_item(*lambda_list, 0);
  ASSERT_NE(lambda_kw_item, nullptr);
  const auto *lambda_kw = as_keyword(*lambda_kw_item);
  ASSERT_NE(lambda_kw, nullptr);
  EXPECT_EQ(*lambda_kw, ast::Keyword::lambda);

  const auto *args_item = list_item(*lambda_list, 1);
  ASSERT_NE(args_item, nullptr);
  const auto *args_list = as_list(*args_item);
  ASSERT_NE(args_list, nullptr);
  ASSERT_EQ(args_list->list.size(), 2U);

  const auto *arg0 = list_item(*args_list, 0);
  ASSERT_NE(arg0, nullptr);
  const auto *arg0_sym = as_symbol(*arg0);
  ASSERT_NE(arg0_sym, nullptr);
  const auto *arg0_name = std::get_if<std::string>(&arg0_sym->value);
  ASSERT_NE(arg0_name, nullptr);
  EXPECT_EQ(*arg0_name, "x");

  const auto *arg1 = list_item(*args_list, 1);
  ASSERT_NE(arg1, nullptr);
  const auto *arg1_sym = as_symbol(*arg1);
  ASSERT_NE(arg1_sym, nullptr);
  const auto *arg1_name = std::get_if<std::string>(&arg1_sym->value);
  ASSERT_NE(arg1_name, nullptr);
  EXPECT_EQ(*arg1_name, "y");

  const auto *body_item = list_item(*lambda_list, 2);
  ASSERT_NE(body_item, nullptr);
  const auto *body_list = as_list(*body_item);
  ASSERT_NE(body_list, nullptr);
  ASSERT_EQ(body_list->list.size(), 3U);

  const auto *op_item = list_item(*body_list, 0);
  ASSERT_NE(op_item, nullptr);
  const auto *op_sym = as_symbol(*op_item);
  ASSERT_NE(op_sym, nullptr);
  const auto *op_name = std::get_if<std::string>(&op_sym->value);
  ASSERT_NE(op_name, nullptr);
  EXPECT_EQ(*op_name, "+");
}

TEST(ParserTests, DefineShorthandDesugarsToLambdaBinding) {
  Parser parser(Lexer("(define (add x y) (+ x y))"));
  ast::AST ast = parser.parse();

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);

  const auto *define_list = as_list(*ast[0]);
  ASSERT_NE(define_list, nullptr);
  ASSERT_EQ(define_list->list.size(), 3U);

  const auto *name_item = list_item(*define_list, 1);
  ASSERT_NE(name_item, nullptr);
  const auto *name_sym = as_symbol(*name_item);
  ASSERT_NE(name_sym, nullptr);
  const auto *name_val = std::get_if<std::string>(&name_sym->value);
  ASSERT_NE(name_val, nullptr);
  EXPECT_EQ(*name_val, "add");

  const auto *lambda_item = list_item(*define_list, 2);
  ASSERT_NE(lambda_item, nullptr);
  const auto *lambda_list = as_list(*lambda_item);
  ASSERT_NE(lambda_list, nullptr);
  ASSERT_EQ(lambda_list->list.size(), 3U);

  const auto *args_item = list_item(*lambda_list, 1);
  ASSERT_NE(args_item, nullptr);
  const auto *args_list = as_list(*args_item);
  ASSERT_NE(args_list, nullptr);
  ASSERT_EQ(args_list->list.size(), 2U);

  const auto *arg0 = list_item(*args_list, 0);
  ASSERT_NE(arg0, nullptr);
  const auto *arg0_sym = as_symbol(*arg0);
  ASSERT_NE(arg0_sym, nullptr);
  const auto *arg0_name = std::get_if<std::string>(&arg0_sym->value);
  ASSERT_NE(arg0_name, nullptr);
  EXPECT_EQ(*arg0_name, "x");

  const auto *arg1 = list_item(*args_list, 1);
  ASSERT_NE(arg1, nullptr);
  const auto *arg1_sym = as_symbol(*arg1);
  ASSERT_NE(arg1_sym, nullptr);
  const auto *arg1_name = std::get_if<std::string>(&arg1_sym->value);
  ASSERT_NE(arg1_name, nullptr);
  EXPECT_EQ(*arg1_name, "y");

  const auto *body_item = list_item(*lambda_list, 2);
  ASSERT_NE(body_item, nullptr);
  const auto *body_list = as_list(*body_item);
  ASSERT_NE(body_list, nullptr);
  ASSERT_EQ(body_list->list.size(), 3U);

  const auto *op_item = list_item(*body_list, 0);
  ASSERT_NE(op_item, nullptr);
  const auto *op_sym = as_symbol(*op_item);
  ASSERT_NE(op_sym, nullptr);
  const auto *op_name = std::get_if<std::string>(&op_sym->value);
  ASSERT_NE(op_name, nullptr);
  EXPECT_EQ(*op_name, "+");
}

TEST(ParserTests, NestedLetInDefineResolves) {
  Parser parser(Lexer("(define (add x y) (let ((z 1)) (+ z y)))"));
  ast::AST ast = parser.parse();

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *define_list = as_list(*ast[0]);
  ASSERT_NE(define_list, nullptr);

  const auto *lambda_item = list_item(*define_list, 2);
  ASSERT_NE(lambda_item, nullptr);
  const auto *lambda_list = as_list(*lambda_item);
  ASSERT_NE(lambda_list, nullptr);

  const auto *body_item = list_item(*lambda_list, 2);
  ASSERT_NE(body_item, nullptr);
  const auto *body_list = as_list(*body_item);
  ASSERT_NE(body_list, nullptr);
  ASSERT_FALSE(body_list->list.empty());

  const auto *fn_item = list_item(*body_list, 0);
  ASSERT_NE(fn_item, nullptr);
  const auto *nested_lambda_list = as_list(*fn_item);
  ASSERT_NE(nested_lambda_list, nullptr);
  const auto *nested_kw_item = list_item(*nested_lambda_list, 0);
  ASSERT_NE(nested_kw_item, nullptr);
  const auto *nested_kw = as_keyword(*nested_kw_item);
  ASSERT_NE(nested_kw, nullptr);
  EXPECT_EQ(*nested_kw, ast::Keyword::lambda);
}

TEST(ParserTests, LetDesugarsToFunctionCall) {
  Parser parser(Lexer("(let ((x 1) (y 2)) (+ x y))"));
  ast::AST ast = parser.parse();
  ast::print_ast(ast);
  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);

  const auto *list = as_list(*ast[0]);
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->list.size(), 2U);

  const auto *fn_item = list_item(*list, 0);
  ASSERT_NE(fn_item, nullptr);
  const auto *lambda_list = as_list(*fn_item);
  ASSERT_NE(lambda_list, nullptr);
  ASSERT_EQ(lambda_list->list.size(), 3U);

  const auto *lambda_kw_item = list_item(*lambda_list, 0);
  ASSERT_NE(lambda_kw_item, nullptr);
  const auto *lambda_kw = as_keyword(*lambda_kw_item);
  ASSERT_NE(lambda_kw, nullptr);
  EXPECT_EQ(*lambda_kw, ast::Keyword::lambda);

  const auto *args_item = list_item(*lambda_list, 1);
  ASSERT_NE(args_item, nullptr);
  const auto *args_list = as_list(*args_item);
  ASSERT_NE(args_list, nullptr);
  ASSERT_EQ(args_list->list.size(), 2U);

  const auto *arg0 = list_item(*args_list, 0);
  ASSERT_NE(arg0, nullptr);
  const auto *arg0_sym = as_symbol(*arg0);
  ASSERT_NE(arg0_sym, nullptr);
  const auto *arg0_name = std::get_if<std::string>(&arg0_sym->value);
  ASSERT_NE(arg0_name, nullptr);
  EXPECT_EQ(*arg0_name, "x");

  const auto *arg1 = list_item(*args_list, 1);
  ASSERT_NE(arg1, nullptr);
  const auto *arg1_sym = as_symbol(*arg1);
  ASSERT_NE(arg1_sym, nullptr);
  const auto *arg1_name = std::get_if<std::string>(&arg1_sym->value);
  ASSERT_NE(arg1_name, nullptr);
  EXPECT_EQ(*arg1_name, "y");

  const auto *body_item = list_item(*lambda_list, 2);
  ASSERT_NE(body_item, nullptr);
  const auto *body_list = as_list(*body_item);
  ASSERT_NE(body_list, nullptr);
  ASSERT_EQ(body_list->list.size(), 3U);

  const auto *values_item = list_item(*list, 1);
  ASSERT_NE(values_item, nullptr);
  const auto *values_list = as_list(*values_item);
  ASSERT_NE(values_list, nullptr);
  ASSERT_EQ(values_list->list.size(), 2U);

  const auto *val0 = list_item(*values_list, 0);
  ASSERT_NE(val0, nullptr);
  const auto *val0_sym = as_symbol(*val0);
  ASSERT_NE(val0_sym, nullptr);
  const auto *val0_num = std::get_if<std::uint64_t>(&val0_sym->value);
  ASSERT_NE(val0_num, nullptr);
  EXPECT_EQ(*val0_num, 1U);

  const auto *val1 = list_item(*values_list, 1);
  ASSERT_NE(val1, nullptr);
  const auto *val1_sym = as_symbol(*val1);
  ASSERT_NE(val1_sym, nullptr);
  const auto *val1_num = std::get_if<std::uint64_t>(&val1_sym->value);
  ASSERT_NE(val1_num, nullptr);
  EXPECT_EQ(*val1_num, 2U);

  ast::print_ast(ast);
}

} // namespace
