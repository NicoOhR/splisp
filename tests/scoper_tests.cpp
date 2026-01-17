#include <string>

#include <gtest/gtest.h>

#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#define private public
#include <frontend/scoper.hpp>
#undef private

namespace {

ast::AST parse_program(const std::string &source) {
  Parser parser((Lexer(source)));
  return parser.parse();
}

TEST(ScoperTests, DefineRegistersFunctionBindingInRoot) {
  ast::AST ast = parse_program("(define add (x y) (+ x y))");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(scoper.table.symbols.size(), 1U);
  const auto it = scoper.table.symbols.find("add");
  ASSERT_NE(it, scoper.table.symbols.end());
  EXPECT_EQ(it->second.kind, BindingKind::FUNC);
  EXPECT_EQ(it->second.value, 0U);
}

TEST(ScoperTests, LambdaDoesNotPolluteRootBindings) {
  ast::AST ast = parse_program("(lambda (x y) (+ x y))");

  Scoper scoper;
  scoper.run(ast);

  EXPECT_TRUE(scoper.table.symbols.empty());
}

TEST(ScoperTests, NestedLambdaScopesAssignSequentialIds) {
  ast::AST ast = parse_program("(lambda (x) (lambda (y z) (+ x y z)))");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(scoper.table.children.size(), 1U);
  const SymbolTable &outer = *scoper.table.children[0];
  ASSERT_EQ(outer.symbols.size(), 1U);
  const auto outer_it = outer.symbols.find("x");
  ASSERT_NE(outer_it, outer.symbols.end());
  EXPECT_EQ(outer_it->second.kind, BindingKind::VALUE);
  EXPECT_EQ(outer_it->second.value, 0U);

  ASSERT_EQ(outer.children.size(), 1U);
  const SymbolTable &inner = *outer.children[0];
  ASSERT_EQ(inner.symbols.size(), 2U);
  const auto y_it = inner.symbols.find("y");
  const auto z_it = inner.symbols.find("z");
  ASSERT_NE(y_it, inner.symbols.end());
  ASSERT_NE(z_it, inner.symbols.end());
  EXPECT_EQ(y_it->second.kind, BindingKind::VALUE);
  EXPECT_EQ(z_it->second.kind, BindingKind::VALUE);
  EXPECT_EQ(y_it->second.value, 1U);
  EXPECT_EQ(z_it->second.value, 2U);
}

TEST(ScoperTests, TopLevelLambdasAssignSequentialIds) {
  ast::AST ast = parse_program("(lambda (a) a) (lambda (b) b)");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(scoper.table.children.size(), 2U);
  const SymbolTable &first = *scoper.table.children[0];
  const SymbolTable &second = *scoper.table.children[1];
  ASSERT_EQ(first.symbols.size(), 1U);
  ASSERT_EQ(second.symbols.size(), 1U);

  const auto a_it = first.symbols.find("a");
  const auto b_it = second.symbols.find("b");
  ASSERT_NE(a_it, first.symbols.end());
  ASSERT_NE(b_it, second.symbols.end());
  EXPECT_EQ(a_it->second.kind, BindingKind::VALUE);
  EXPECT_EQ(b_it->second.kind, BindingKind::VALUE);
  EXPECT_EQ(a_it->second.value, 0U);
  EXPECT_EQ(b_it->second.value, 1U);
}

} // namespace
