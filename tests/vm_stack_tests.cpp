#include <cstdint>
#include <optional>
#include <stack>
#include <vector>

#include <gtest/gtest.h>

#include <backend/isa/isa.hpp>
#include <backend/vm/stack.hpp>

struct StackTestAccess {
  static MachineState runInstruction(Stack &stack) {
    return stack.runInstruction();
  }

  static std::stack<std::unique_ptr<Cell>> &data(Stack &stack) {
    return stack.data_stack;
  }
  static std::stack<std::unique_ptr<Cell>> &returns(Stack &stack) {
    return stack.return_stack;
  }
  static std::map<core::SymbolId, std::unique_ptr<Cell>> &globals(Stack &stack) {
    return stack.global_tbl;
  }
  static size_t &pc(Stack &stack) { return stack.pc; }
};

namespace {

constexpr size_t kInstrSize = 9;

Stack make_stack(ISA::Operation op, std::optional<uint64_t> operand = {}) {
  ISA::Instruction instr{op, operand};
  std::vector<ISA::Instruction> program{instr};
  std::vector<uint8_t> data{};
  return Stack(std::move(program), std::move(data));
}

Stack make_stack_with_data(ISA::Operation op, std::optional<uint64_t> operand,
                           std::vector<uint8_t> data) {
  ISA::Instruction instr{op, operand};
  std::vector<ISA::Instruction> program{instr};
  return Stack(std::move(program), std::move(data));
}

TEST(StackTests, DispatchArithmeticAdd) {
  auto stack = make_stack(ISA::Operation::ADD);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{2}));
  data.push(std::make_unique<Cell>(Cell{3}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 5U);
}

TEST(StackTests, DispatchArithmeticOpsCoarse) {
  auto run_binary = [](ISA::Operation op, uint64_t b, uint64_t a) -> uint64_t {
    auto stack = make_stack(op);
    auto &data = StackTestAccess::data(stack);
    data.push(std::make_unique<Cell>(Cell{b}));
    data.push(std::make_unique<Cell>(Cell{a}));
    auto state = StackTestAccess::runInstruction(stack);
    // gtest functionality is not available inside lambdas
    return data.top()->value;
  };

  EXPECT_EQ(run_binary(ISA::Operation::ADD, 2, 3), 5U);
  EXPECT_EQ(run_binary(ISA::Operation::SUB, 3, 10), 7U);
  EXPECT_EQ(run_binary(ISA::Operation::MUL, 7, 6), 42U);
  EXPECT_EQ(run_binary(ISA::Operation::DIV, 4, 20), 5U);
  EXPECT_EQ(run_binary(ISA::Operation::MOD, 7, 20), 6U);
}

TEST(StackTests, DispatchLogicLt) {
  auto stack = make_stack(ISA::Operation::LT);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{2}));
  data.push(std::make_unique<Cell>(Cell{1}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 1U);
}

TEST(StackTests, DispatchTransferPush) {
  auto stack = make_stack(ISA::Operation::PUSH, 42);
  auto &data = StackTestAccess::data(stack);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 42U);
}

TEST(StackTests, DispatchControlHalt) {
  auto stack = make_stack(ISA::Operation::HALT);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::HALT);
}

TEST(StackTests, DispatchControlWait) {
  auto stack = make_stack(ISA::Operation::WAIT);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{5}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(data.size(), 1U);
}

TEST(StackTests, DispatchControlJmp) {
  auto stack = make_stack(ISA::Operation::JMP);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{7}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 7U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpTaken) {
  auto stack = make_stack(ISA::Operation::CJMP);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{9}));
  data.push(std::make_unique<Cell>(Cell{1}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 9U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpNotTaken) {
  auto stack = make_stack(ISA::Operation::CJMP);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{9}));
  data.push(std::make_unique<Cell>(Cell{0}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 0U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlRet) {
  auto stack = make_stack(ISA::Operation::RET);
  auto &returns = StackTestAccess::returns(stack);
  returns.push(std::make_unique<Cell>(Cell{4}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 4U);
  EXPECT_EQ(returns.size(), 0U);
}

TEST(StackTests, DispatchTransferRot) {
  auto stack = make_stack(ISA::Operation::ROT);
  auto &data = StackTestAccess::data(stack);
  const uint64_t a = 1;
  const uint64_t b = 2;
  const uint64_t c = 3;
  data.push(std::make_unique<Cell>(Cell{c}));
  data.push(std::make_unique<Cell>(Cell{b}));
  data.push(std::make_unique<Cell>(Cell{a}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data.top()->value, b);
  data.pop();
  EXPECT_EQ(data.top()->value, c);
  data.pop();
  EXPECT_EQ(data.top()->value, a);
}

TEST(StackTests, DispatchTransferTuck) {
  auto stack = make_stack(ISA::Operation::TUCK);
  auto &data = StackTestAccess::data(stack);
  const uint64_t a = 20;
  const uint64_t b = 10;
  const uint64_t c = 7;
  data.push(std::make_unique<Cell>(Cell{c}));
  data.push(std::make_unique<Cell>(Cell{b}));
  data.push(std::make_unique<Cell>(Cell{a}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data.top()->value, c);
  data.pop();
  EXPECT_EQ(data.top()->value, a);
  data.pop();
  EXPECT_EQ(data.top()->value, b);
}

TEST(StackTests, DispatchTransferFetch) {
  std::vector<uint8_t> mem{0x11, 0x22, 0x33};
  auto stack = make_stack_with_data(ISA::Operation::FETCH, std::nullopt, mem);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{9}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 0x2211U);
}

TEST(StackTests, DispatchControlMkClosureCallRestoresCapturedOrder) {
  std::vector<ISA::Instruction> program{
      {ISA::Operation::MKCLOSURE, 123},
      {ISA::Operation::CALL, std::nullopt},
  };
  std::vector<uint8_t> data_bytes{};
  Stack stack(std::move(program), std::move(data_bytes));

  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{4}));
  data.push(std::make_unique<Cell>(Cell{6}));
  data.push(std::make_unique<Cell>(Cell{2}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 0U);

  StackTestAccess::pc(stack) += kInstrSize;
  state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 123U);
  ASSERT_EQ(data.size(), 2U);
  EXPECT_EQ(data.top()->value, 6U);
  data.pop();
  EXPECT_EQ(data.top()->value, 4U);
}

TEST(StackTests, DispatchControlMkClosureConsumesCapturesAndLeavesHandle) {
  auto stack = make_stack(ISA::Operation::MKCLOSURE, 321);
  auto &data = StackTestAccess::data(stack);
  data.push(std::make_unique<Cell>(Cell{99}));
  data.push(std::make_unique<Cell>(Cell{4}));
  data.push(std::make_unique<Cell>(Cell{6}));
  data.push(std::make_unique<Cell>(Cell{2}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 2U);
  EXPECT_TRUE(data.top()->function);
  EXPECT_EQ(data.top()->value, 0U);
  data.pop();
  EXPECT_EQ(data.top()->value, 99U);
}

TEST(StackTests, DispatchControlMutGlobalStoresPoppedValueByOperand) {
  auto stack = make_stack(ISA::Operation::MUTGLOBAL, 77);
  auto &data = StackTestAccess::data(stack);
  auto &globals = StackTestAccess::globals(stack);
  data.push(std::make_unique<Cell>(Cell{42}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_TRUE(data.empty());
  ASSERT_EQ(globals.count(77), 1U);
  ASSERT_NE(globals[77], nullptr);
  EXPECT_EQ(globals[77]->value, 42U);
  EXPECT_FALSE(globals[77]->function);
}

TEST(StackTests, DispatchControlLoadGlobalPushesBoundValue) {
  auto stack = make_stack(ISA::Operation::LOADGLOBAL, 77);
  auto &data = StackTestAccess::data(stack);
  auto &globals = StackTestAccess::globals(stack);
  globals[77] = std::make_unique<Cell>(Cell{42, false});

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top()->value, 42U);
  EXPECT_FALSE(data.top()->function);
  ASSERT_EQ(globals.count(77), 1U);
  ASSERT_NE(globals[77], nullptr);
  EXPECT_EQ(globals[77]->value, 42U);
  EXPECT_FALSE(globals[77]->function);
  EXPECT_EQ(StackTestAccess::pc(stack), 0U);
  EXPECT_TRUE(StackTestAccess::returns(stack).empty());
}

TEST(StackTests, DispatchControlLoadGlobalClonesBoundCell) {
  auto stack = make_stack(ISA::Operation::LOADGLOBAL, 77);
  auto &data = StackTestAccess::data(stack);
  auto &globals = StackTestAccess::globals(stack);
  globals[77] = std::make_unique<Cell>(Cell{42, true});
  const auto *stored_ptr = globals[77].get();

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  ASSERT_NE(data.top().get(), nullptr);
  EXPECT_NE(data.top().get(), stored_ptr);
  EXPECT_EQ(data.top()->value, 42U);
  EXPECT_TRUE(data.top()->function);
  ASSERT_NE(globals[77], nullptr);
  EXPECT_EQ(globals[77].get(), stored_ptr);
  EXPECT_EQ(globals[77]->value, 42U);
  EXPECT_TRUE(globals[77]->function);
}

} // namespace
