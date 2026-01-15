#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <frontend/ast.hpp>
#include <frontend/core.hpp>

namespace {

std::unique_ptr<ast::SExp> make_keyword(ast::Keyword keyword) {
  return std::make_unique<ast::SExp>(ast::SExp{.node = ast::Symbol{keyword}});
}

std::unique_ptr<ast::SExp> make_symbol_id(std::uint64_t id) {
  return std::make_unique<ast::SExp>(
      ast::SExp{.node = ast::Symbol{ast::SymbolID{id}}});
}

std::unique_ptr<ast::SExp> make_number(std::uint64_t value) {
  return std::make_unique<ast::SExp>(
      ast::SExp{.node = ast::Symbol{value}});
}

std::unique_ptr<ast::SExp> make_bool(bool value) {
  return std::make_unique<ast::SExp>(ast::SExp{.node = ast::Symbol{value}});
}

std::unique_ptr<ast::SExp>
make_list(std::vector<std::unique_ptr<ast::SExp>> items) {
  ast::List list;
  for (auto &item : items) {
    list.list.push_back(std::move(item));
  }
  return std::make_unique<ast::SExp>(ast::SExp{.node = std::move(list)});
}

const core::Const *as_const(const core::Expr &expr) {
  return std::get_if<core::Const>(&expr.node);
}

const core::Var *as_var(const core::Expr &expr) {
  return std::get_if<core::Var>(&expr.node);
}

TEST(LowererTests, IfLowersToCondWithConsts) {
  ast::AST ast;
  std::vector<std::unique_ptr<ast::SExp>> items;
  items.push_back(make_keyword(ast::Keyword::if_expr));
  items.push_back(make_bool(true));
  items.push_back(make_number(1));
  items.push_back(make_number(0));
  ast.push_back(make_list(std::move(items)));

  core::Lowerer lowerer;
  const core::Program &program = lowerer.lower(ast);

  ASSERT_EQ(program.size(), 1U);
  const auto *expr = std::get_if<core::Expr>(&program[0]);
  ASSERT_NE(expr, nullptr);
  const auto *cond = std::get_if<core::Cond>(&expr->node);
  ASSERT_NE(cond, nullptr);
  ASSERT_NE(cond->condition, nullptr);
  ASSERT_NE(cond->then, nullptr);
  ASSERT_NE(cond->otherwise, nullptr);

  const auto *condition_const = as_const(*cond->condition);
  const auto *then_const = as_const(*cond->then);
  const auto *else_const = as_const(*cond->otherwise);
  ASSERT_NE(condition_const, nullptr);
  ASSERT_NE(then_const, nullptr);
  ASSERT_NE(else_const, nullptr);
  EXPECT_EQ(condition_const->value, 1U);
  EXPECT_EQ(then_const->value, 1U);
  EXPECT_EQ(else_const->value, 0U);
}

TEST(LowererTests, DefineLowersToLambdaWithBodies) {
  ast::AST ast;

  std::vector<std::unique_ptr<ast::SExp>> args;
  args.push_back(make_symbol_id(1));
  args.push_back(make_symbol_id(2));

  std::vector<std::unique_ptr<ast::SExp>> apply_items;
  apply_items.push_back(make_symbol_id(77));
  apply_items.push_back(make_symbol_id(1));

  std::vector<std::unique_ptr<ast::SExp>> lambda_items;
  lambda_items.push_back(make_keyword(ast::Keyword::lambda));
  lambda_items.push_back(make_list(std::move(args)));
  lambda_items.push_back(make_list(std::move(apply_items)));
  lambda_items.push_back(make_number(42));

  std::vector<std::unique_ptr<ast::SExp>> define_items;
  define_items.push_back(make_keyword(ast::Keyword::define));
  define_items.push_back(make_symbol_id(10));
  define_items.push_back(make_list(std::move(lambda_items)));

  ast.push_back(make_list(std::move(define_items)));

  core::Lowerer lowerer;
  const core::Program &program = lowerer.lower(ast);

  ASSERT_EQ(program.size(), 1U);
  const auto *defn = std::get_if<core::Define>(&program[0]);
  ASSERT_NE(defn, nullptr);
  EXPECT_EQ(defn->name, 10U);
  ASSERT_NE(defn->rhs, nullptr);

  const auto *lambda = std::get_if<core::Lambda>(&defn->rhs->node);
  ASSERT_NE(lambda, nullptr);
  ASSERT_EQ(lambda->formals.size(), 2U);
  ASSERT_NE(lambda->formals[0], nullptr);
  ASSERT_NE(lambda->formals[1], nullptr);
  EXPECT_EQ(*lambda->formals[0], 1U);
  EXPECT_EQ(*lambda->formals[1], 2U);

  ASSERT_EQ(lambda->body.size(), 2U);
  ASSERT_NE(lambda->body[0], nullptr);
  ASSERT_NE(lambda->body[1], nullptr);

  const auto *apply = std::get_if<core::Apply>(&lambda->body[0]->node);
  ASSERT_NE(apply, nullptr);
  ASSERT_NE(apply->callee, nullptr);
  const auto *callee_var = as_var(*apply->callee);
  ASSERT_NE(callee_var, nullptr);
  EXPECT_EQ(callee_var->id, 77U);
  ASSERT_EQ(apply->args.size(), 1U);
  ASSERT_NE(apply->args[0], nullptr);
  const auto *arg_var = as_var(*apply->args[0]);
  ASSERT_NE(arg_var, nullptr);
  EXPECT_EQ(arg_var->id, 1U);

  const auto *tail_const = as_const(*lambda->body[1]);
  ASSERT_NE(tail_const, nullptr);
  EXPECT_EQ(tail_const->value, 42U);
}

} // namespace
