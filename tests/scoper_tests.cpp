#include <cstddef>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#define private public
#include <frontend/scoper.hpp>
#undef private

namespace {

const ast::List *as_list(const ast::SExp &sexp) {
  return std::get_if<ast::List>(&sexp.node);
}

const ast::Symbol *as_symbol(const ast::SExp &sexp) {
  return std::get_if<ast::Symbol>(&sexp.node);
}

const ast::SymbolID *as_symbol_id(const ast::SExp &sexp) {
  const auto *sym = as_symbol(sexp);
  if (!sym) {
    return nullptr;
  }
  return std::get_if<ast::SymbolID>(&sym->value);
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

ast::AST parse_program(const std::string &source) {
  Parser parser((Lexer(source)));
  return parser.parse();
}

TEST(ScoperTests, DefineRegistersFunctionBindingInRoot) {
  ast::AST ast = parse_program("(define add (x y) (+ x y))");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(scoper.root.symbols.size(), 1U);
  const auto it = scoper.root.symbols.find("add");
  ASSERT_NE(it, scoper.root.symbols.end());
  EXPECT_EQ(it->second.kind, BindingKind::FUNC);
  EXPECT_EQ(it->second.value, 0U);
}

TEST(ScoperTests, LambdaDoesNotPolluteRootBindings) {
  ast::AST ast = parse_program("(lambda (x y) (+ x y))");

  Scoper scoper;
  scoper.run(ast);

  EXPECT_TRUE(scoper.root.symbols.empty());
}

TEST(ScoperTests, NestedLambdaScopesAssignSequentialIds) {
  ast::AST ast = parse_program("(lambda (x) (lambda (y z) (+ x y z)))");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(scoper.root.children.size(), 1U);
  const SymbolTable &outer = *scoper.root.children[0];
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

  ASSERT_EQ(scoper.root.children.size(), 2U);
  const SymbolTable &first = *scoper.root.children[0];
  const SymbolTable &second = *scoper.root.children[1];
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

TEST(ScoperTests, LambdaScopeIdsAnnotatedOnAstNodes) {
  ast::AST ast = parse_program("(lambda (x) (lambda (y) (+ x y)))");

  Scoper scoper;
  scoper.run(ast);

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *outer_list = as_list(*ast[0]);
  ASSERT_NE(outer_list, nullptr);
  const auto *outer_kw_item = list_item(*outer_list, 0);
  ASSERT_NE(outer_kw_item, nullptr);
  const auto *outer_kw = as_keyword(*outer_kw_item);
  ASSERT_NE(outer_kw, nullptr);
  EXPECT_EQ(*outer_kw, ast::Keyword::lambda);

  ASSERT_TRUE(ast[0]->scope_id.has_value());
  ASSERT_EQ(scoper.root.children.size(), 1U);
  const SymbolTable &outer_scope = *scoper.root.children[0];
  EXPECT_EQ(ast[0]->scope_id.value(), outer_scope.scope_id);

  const auto *inner_item = list_item(*outer_list, 2);
  ASSERT_NE(inner_item, nullptr);
  const auto *inner_list = as_list(*inner_item);
  ASSERT_NE(inner_list, nullptr);
  const auto *inner_kw_item = list_item(*inner_list, 0);
  ASSERT_NE(inner_kw_item, nullptr);
  const auto *inner_kw = as_keyword(*inner_kw_item);
  ASSERT_NE(inner_kw, nullptr);
  EXPECT_EQ(*inner_kw, ast::Keyword::lambda);

  ASSERT_TRUE(inner_item->scope_id.has_value());
  ASSERT_EQ(outer_scope.children.size(), 1U);
  const SymbolTable &inner_scope = *outer_scope.children[0];
  EXPECT_EQ(inner_item->scope_id.value(), inner_scope.scope_id);
}

TEST(ScoperTests, SearchFindsNearestBindingInParentScopes) {
  ast::AST ast = parse_program("(lambda (x) (lambda (y) (+ x y)))");

  Scoper scoper;
  scoper.run(ast);

  std::ostringstream table_out;
  print_symbol_table(table_out, scoper.root);
  std::cout << table_out.str();
  EXPECT_FALSE(table_out.str().empty());

  ASSERT_EQ(scoper.root.children.size(), 1U);
  const SymbolTable &outer = *scoper.root.children[0];
  ASSERT_EQ(outer.children.size(), 1U);
  const SymbolTable &inner = *outer.children[0];

  const Binding inner_binding = scoper.search("y", inner.scope_id);
  EXPECT_EQ(inner_binding.kind, BindingKind::VALUE);
  EXPECT_EQ(inner_binding.value, 1U);

  const Binding outer_binding = scoper.search("x", inner.scope_id);
  EXPECT_EQ(outer_binding.kind, BindingKind::VALUE);
  EXPECT_EQ(outer_binding.value, 0U);
}

TEST(ScoperTests, ResolveRewritesSymbolsToBindingIds) {
  ast::AST ast = parse_program("(lambda (x) x)");

  Scoper scoper;
  scoper.run(ast);
  scoper.resolve(ast);

  ASSERT_EQ(ast.size(), 1U);
  ASSERT_NE(ast[0], nullptr);
  const auto *outer_list = as_list(*ast[0]);
  ASSERT_NE(outer_list, nullptr);
  const auto *body_item = list_item(*outer_list, 2);
  ASSERT_NE(body_item, nullptr);
  const auto *body_id = as_symbol_id(*body_item);
  ASSERT_NE(body_id, nullptr);
  EXPECT_EQ(body_id->id, 0U);
}

} // namespace
