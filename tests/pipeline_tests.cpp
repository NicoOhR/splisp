#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <backend/generator/generator.hpp>
#include <backend/vm/stack.hpp>
#include <frontend/core.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/scoper.hpp>

struct StackTestAccess {
  static std::vector<std::shared_ptr<Cell>> &data(Stack &stack) {
    return stack.data_stack;
  }
};

namespace {

struct RunResult {
  MachineState state;
  int64_t value;
};

RunResult run(const std::string &src) {
  Lexer lex(src);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  Scoper scoper;
  scoper.run(ast);
  scoper.resolve(ast);
  core::Lowerer lowerer;
  const core::Program &ir = lowerer.lower(ast);
  Generator gen(ir);
  auto bc = gen.generate();
  Stack vm(bc, {});
  auto state = vm.run_program();
  auto &data = StackTestAccess::data(vm);
  int64_t val = data.empty() ? 0 : data.back()->value;
  return {state, val};
}

} // namespace

// ── Arithmetic builtins ────────────────────────────────────────────────────

TEST(PipelineTests, Add) {
  auto [state, val] = run("(+ 1 2)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 3);
}

TEST(PipelineTests, Sub) {
  auto [state, val] = run("(- 10 3)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 7);
}

TEST(PipelineTests, Mul) {
  auto [state, val] = run("(* 3 4)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 12);
}

TEST(PipelineTests, Div) {
  auto [state, val] = run("(/ 10 2)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 5);
}

TEST(PipelineTests, Mod) {
  auto [state, val] = run("(% 10 3)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 1);
}

TEST(PipelineTests, NestedArithmetic) {
  auto [state, val] = run("(+ (* 2 3) (- 10 4))");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 12);
}

// ── Conditionals ───────────────────────────────────────────────────────────

TEST(PipelineTests, CondTaken) {
  auto [state, val] = run("(if 1 42 0)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 42);
}

TEST(PipelineTests, CondNotTaken) {
  auto [state, val] = run("(if 0 0 99)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 99);
}

TEST(PipelineTests, CondWithComputedCondition) {
  auto [state, val] = run("(if (+ 1 0) 1 0)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 1);
}

// ── Immediately applied lambdas ────────────────────────────────────────────

TEST(PipelineTests, LambdaIdentity) {
  auto [state, val] = run("((lambda (x) x) 7)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 7);
}

TEST(PipelineTests, LambdaSquare) {
  auto [state, val] = run("((lambda (x) (* x x)) 5)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 25);
}

TEST(PipelineTests, LambdaTwoArgs) {
  auto [state, val] = run("((lambda (x y) (+ x y)) 3 4)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 7);
}

TEST(PipelineTests, LambdaWithCond) {
  auto [state, val] = run("((lambda (x) (if x 1 0)) 0)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 0);
}

// ── Top-level define + call ────────────────────────────────────────────────

TEST(PipelineTests, DefineAndCallSquare) {
  auto [state, val] = run("(define (square x) (* x x)) (square 5)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 25);
}

TEST(PipelineTests, DefineAndCallTwoArgs) {
  auto [state, val] = run("(define (add x y) (+ x y)) (add 3 4)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 7);
}

TEST(PipelineTests, DefineAndCallWithCond) {
  auto [state, val] = run("(define (abs-val x) (if x x 0)) (abs-val 5)");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 5);
}

// ── Mutual recursion via letrec ────────────────────────────────────────────

TEST(PipelineTests, LetrecEvenOdd) {
  auto [state, val] = run(R"(
    (letrec ((even? (lambda (n) (if n (odd?  (- n 1)) 1)))
             (odd?  (lambda (n) (if n (even? (- n 1)) 0))))
      (even? 4))
  )");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 1); // 4 is even
}

TEST(PipelineTests, LetrecEvenOddOdd) {
  auto [state, val] = run(R"(
    (letrec ((even? (lambda (n) (if n (odd?  (- n 1)) 1)))
             (odd?  (lambda (n) (if n (even? (- n 1)) 0))))
      (odd? 3))
  )");
  EXPECT_EQ(state, MachineState::HALT);
  EXPECT_EQ(val, 1); // 3 is odd
}
