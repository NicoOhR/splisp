#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <frontend/core.hpp>

struct GeneratorTestAccess {
  static std::vector<ISA::Instruction> &bytecode(Generator &gen) {
    return gen.bytecode;
  }
};

namespace {

// Builtin symbol IDs — order from Scoper constructor: +, -, *, /, %
constexpr core::SymbolId kAdd = 0;
constexpr core::SymbolId kSub = 1;
constexpr core::SymbolId kMul = 2;
constexpr core::SymbolId kDiv = 3;
constexpr core::SymbolId kMod = 4;

core::Expr const_expr(uint64_t v) { return core::Expr{.node = core::Const{v}}; }

core::Expr var_expr(core::SymbolId id) {
  return core::Expr{.node = core::Var{id}};
}

bool has_op(const std::vector<ISA::Instruction> &bc, ISA::Operation op) {
  return std::any_of(bc.begin(), bc.end(),
                     [op](const ISA::Instruction &i) { return i.op == op; });
}

bool has_push(const std::vector<ISA::Instruction> &bc, uint64_t val) {
  return std::any_of(bc.begin(), bc.end(), [val](const ISA::Instruction &i) {
    return i.op == ISA::Operation::PUSH && i.operand == val;
  });
}

std::optional<size_t> first_index(const std::vector<ISA::Instruction> &bc,
                                  ISA::Operation op) {
  for (size_t i = 0; i < bc.size(); i++) {
    if (bc[i].op == op)
      return i;
  }
  return std::nullopt;
}

} // namespace

TEST(GeneratorTests, EmitConstProducesPush) {
  core::Program prog;
  prog.emplace_back(const_expr(42));
  Generator gen(prog);
  gen.generate();
  EXPECT_TRUE(has_push(GeneratorTestAccess::bytecode(gen), 42));
}

TEST(GeneratorTests, EmitTopDefineConstPushThenMkglobal) {
  core::Program prog;
  prog.emplace_back(core::Define{
      .name = 7, .rhs = std::make_unique<core::Expr>(const_expr(55))});
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);

  auto push_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::PUSH && i.operand == 55;
      });
  auto mkglobal_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::MKGLOBAL && i.operand == 7;
      });

  ASSERT_NE(push_it, bc.end());
  ASSERT_NE(mkglobal_it, bc.end());
  EXPECT_LT(push_it, mkglobal_it);
}

TEST(GeneratorTests, EmitVarGlobalEmitsLoadglobal) {
  core::Program prog;
  prog.emplace_back(core::Define{
      .name = 10, .rhs = std::make_unique<core::Expr>(const_expr(10))});
  prog.emplace_back(var_expr(10));
  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  EXPECT_TRUE(std::any_of(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
    return i.op == ISA::Operation::LOADGLOBAL && i.operand == 10;
  }));
}

TEST(GeneratorTests, EmitApplyAddEmitsArgsInOrderThenAdd) {
  core::Apply apply;
  apply.callee = std::make_unique<core::Expr>(var_expr(kAdd));
  apply.args.push_back(std::make_unique<core::Expr>(const_expr(3)));
  apply.args.push_back(std::make_unique<core::Expr>(const_expr(4)));

  core::Program prog;
  prog.emplace_back(core::Expr{.node = std::move(apply)});
  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  ASSERT_TRUE(has_push(bc, 3));
  ASSERT_TRUE(has_push(bc, 4));
  ASSERT_TRUE(has_op(bc, ISA::Operation::ADD));

  size_t add_idx = *first_index(bc, ISA::Operation::ADD);
  auto push3 =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::PUSH && i.operand == 3;
      });
  auto push4 =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::PUSH && i.operand == 4;
      });
  EXPECT_LT((size_t)(push3 - bc.begin()), add_idx);
  EXPECT_LT((size_t)(push4 - bc.begin()), add_idx);
  EXPECT_LT(push3, push4); // left arg before right arg
}

TEST(GeneratorTests, BuiltinOpsMapToCorrectInstructions) {
  const std::pair<core::SymbolId, ISA::Operation> cases[] = {
      {kSub, ISA::Operation::SUB},
      {kMul, ISA::Operation::MUL},
      {kDiv, ISA::Operation::DIV},
      {kMod, ISA::Operation::MOD},
  };

  for (auto [id, expected_op] : cases) {
    core::Apply apply;
    apply.callee = std::make_unique<core::Expr>(var_expr(id));
    apply.args.push_back(std::make_unique<core::Expr>(const_expr(10)));
    apply.args.push_back(std::make_unique<core::Expr>(const_expr(2)));

    core::Program prog;
    prog.emplace_back(core::Expr{.node = std::move(apply)});
    core::print_program(prog);
    Generator gen(prog);
    gen.generate();
    auto &bc = GeneratorTestAccess::bytecode(gen);
    print_bytecode(bc);
    EXPECT_TRUE(has_op(bc, expected_op));
  }
}

TEST(GeneratorTests, EmitApplyUserFuncEmitsArgsThenLoadglobalThenCall) {
  core::Lambda lam;
  lam.formals.push_back(std::make_unique<core::SymbolId>(20));
  lam.body.push_back(std::make_unique<core::Expr>(var_expr(20)));

  core::Program prog;
  prog.emplace_back(core::Define{
      .name = 10,
      .rhs = std::make_unique<core::Expr>(core::Expr{.node = std::move(lam)})});

  core::Apply call;
  call.callee = std::make_unique<core::Expr>(var_expr(10));
  call.args.push_back(std::make_unique<core::Expr>(const_expr(42)));
  prog.emplace_back(core::Expr{.node = std::move(call)});

  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  EXPECT_TRUE(has_push(bc, 42));

  auto loadglobal_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::LOADGLOBAL && i.operand == 10;
      });
  auto call_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::CALL;
      });

  ASSERT_NE(loadglobal_it, bc.end());
  ASSERT_NE(call_it, bc.end());
  EXPECT_LT(loadglobal_it, call_it);
}

TEST(GeneratorTests, EmitCondHasCjmpWithConditionAndBothBranchValues) {
  core::Cond cond;
  cond.condition = std::make_unique<core::Expr>(const_expr(1));
  cond.then = std::make_unique<core::Expr>(const_expr(10));
  cond.otherwise = std::make_unique<core::Expr>(const_expr(20));

  core::Program prog;
  prog.emplace_back(core::Expr{.node = std::move(cond)});
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);

  EXPECT_TRUE(has_op(bc, ISA::Operation::CJMP));
  EXPECT_TRUE(has_push(bc, 10));
  EXPECT_TRUE(has_push(bc, 20));

  // condition must be evaluated before CJMP
  size_t cjmp_idx = *first_index(bc, ISA::Operation::CJMP);
  auto cond_push =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::PUSH && i.operand == 1;
      });
  ASSERT_NE(cond_push, bc.end());
  EXPECT_LT((size_t)(cond_push - bc.begin()), cjmp_idx);
}

TEST(GeneratorTests, EmitLambdaHasMkClosurePointingToEnter) {
  core::Lambda lam;
  lam.formals.push_back(std::make_unique<core::SymbolId>(1));
  lam.formals.push_back(std::make_unique<core::SymbolId>(2));
  lam.body.push_back(std::make_unique<core::Expr>(const_expr(42)));

  core::Program prog;
  prog.emplace_back(core::Expr{.node = std::move(lam)});
  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  ASSERT_TRUE(has_op(bc, ISA::Operation::MKCLOSURE));
  ASSERT_TRUE(has_op(bc, ISA::Operation::ENTER));
  EXPECT_TRUE(has_op(bc, ISA::Operation::RET));
  EXPECT_TRUE(has_push(bc, 42));

  // MKCLOSURE's operand is the byte offset of the lambda body.
  // The first instruction of the body must be ENTER.
  size_t mkclosure_idx = *first_index(bc, ISA::Operation::MKCLOSURE);
  uint64_t body_byte_offset = bc[mkclosure_idx].operand.value();
  size_t body_instr_idx = body_byte_offset / 9;
  ASSERT_LT(body_instr_idx, bc.size());
  EXPECT_EQ(bc[body_instr_idx].op, ISA::Operation::ENTER);
}

TEST(GeneratorTests, EmitLambdaEnterOperandEqualsFormals) {
  // lambda with 2 formals at top level (no captures) → ENTER 2
  core::Lambda lam;
  lam.formals.push_back(std::make_unique<core::SymbolId>(1));
  lam.formals.push_back(std::make_unique<core::SymbolId>(2));
  lam.body.push_back(std::make_unique<core::Expr>(const_expr(0)));

  core::Program prog;
  prog.emplace_back(core::Expr{.node = std::move(lam)});
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);

  EXPECT_TRUE(std::any_of(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
    return i.op == ISA::Operation::ENTER && i.operand == 2;
  }));
}

TEST(GeneratorTests, EmitLambdaBodyVarUsesGetLocal) {
  // (lambda (x y) y) — y is formal at index 1 → GET_LOCAL 1
  core::Lambda lam;
  lam.formals.push_back(std::make_unique<core::SymbolId>(10)); // x → slot 0
  lam.formals.push_back(std::make_unique<core::SymbolId>(11)); // y → slot 1
  lam.body.push_back(std::make_unique<core::Expr>(var_expr(11)));

  core::Program prog;
  prog.emplace_back(core::Expr{.node = std::move(lam)});
  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  EXPECT_TRUE(std::any_of(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
    return i.op == ISA::Operation::GETLOCAL && i.operand == 1;
  }));
}

TEST(GeneratorTests, EmitTopDefineLambdaMkglobalComesAfterBody) {
  core::Lambda lam;
  lam.formals.push_back(std::make_unique<core::SymbolId>(5));
  lam.body.push_back(std::make_unique<core::Expr>(const_expr(0)));

  core::Program prog;
  prog.emplace_back(core::Define{
      .name = 15,
      .rhs = std::make_unique<core::Expr>(core::Expr{.node = std::move(lam)})});
  core::print_program(prog);
  Generator gen(prog);
  gen.generate();
  auto &bc = GeneratorTestAccess::bytecode(gen);
  print_bytecode(bc);

  EXPECT_TRUE(has_op(bc, ISA::Operation::MKCLOSURE));

  auto mkglobal_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::MKGLOBAL && i.operand == 15;
      });
  auto mkclosure_it =
      std::find_if(bc.begin(), bc.end(), [](const ISA::Instruction &i) {
        return i.op == ISA::Operation::MKCLOSURE;
      });
  ASSERT_NE(mkglobal_it, bc.end());
  ASSERT_NE(mkclosure_it, bc.end());
  // MKCLOSURE (which pushes the closure handle) must precede MKGLOBAL (which
  // stores it)
  EXPECT_LT(mkclosure_it, mkglobal_it);
}
