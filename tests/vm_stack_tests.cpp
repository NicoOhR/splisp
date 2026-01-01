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

  static std::stack<uint64_t> &data(Stack &stack) { return stack.data_stack; }
  static std::stack<uint64_t> &returns(Stack &stack) {
    return stack.return_stack;
  }
  static size_t &pc(Stack &stack) { return stack.pc; }
};

namespace {

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
  data.push(2);
  data.push(3);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top(), 5U);
}

TEST(StackTests, DispatchArithmeticOpsCoarse) {
  auto run_binary = [](ISA::Operation op, uint64_t b, uint64_t a) -> uint64_t {
    auto stack = make_stack(op);
    auto &data = StackTestAccess::data(stack);
    data.push(b);
    data.push(a);
    auto state = StackTestAccess::runInstruction(stack);
    // gtest functionality is not available inside lambdas
    return data.top();
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
  data.push(2);
  data.push(1);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top(), 1U);
}

TEST(StackTests, DispatchTransferPush) {
  auto stack = make_stack(ISA::Operation::PUSH, 42);
  auto &data = StackTestAccess::data(stack);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top(), 42U);
}

TEST(StackTests, DispatchControlHalt) {
  auto stack = make_stack(ISA::Operation::HALT);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::HALT);
}

TEST(StackTests, DispatchControlWait) {
  auto stack = make_stack(ISA::Operation::WAIT);
  auto &data = StackTestAccess::data(stack);
  data.push(5);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(data.size(), 1U);
}

TEST(StackTests, DispatchControlJmp) {
  auto stack = make_stack(ISA::Operation::JMP);
  auto &data = StackTestAccess::data(stack);
  data.push(7);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 7U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpTaken) {
  auto stack = make_stack(ISA::Operation::CJMP);
  auto &data = StackTestAccess::data(stack);
  data.push(9);
  data.push(1);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 9U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpNotTaken) {
  auto stack = make_stack(ISA::Operation::CJMP);
  auto &data = StackTestAccess::data(stack);
  data.push(9);
  data.push(0);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 0U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCall) {
  auto stack = make_stack(ISA::Operation::CALL);
  auto &data = StackTestAccess::data(stack);
  auto &returns = StackTestAccess::returns(stack);
  data.push(12);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(returns.size(), 1U);
  EXPECT_EQ(returns.top(), 0U);
  EXPECT_EQ(StackTestAccess::pc(stack), 12U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlRet) {
  auto stack = make_stack(ISA::Operation::RET);
  auto &returns = StackTestAccess::returns(stack);
  returns.push(4);

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
  data.push(c);
  data.push(b);
  data.push(a);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data.top(), b);
  data.pop();
  EXPECT_EQ(data.top(), c);
  data.pop();
  EXPECT_EQ(data.top(), a);
}

TEST(StackTests, DispatchTransferTuck) {
  auto stack = make_stack(ISA::Operation::TUCK);
  auto &data = StackTestAccess::data(stack);
  const uint64_t a = 20;
  const uint64_t b = 10;
  const uint64_t c = 7;
  data.push(c);
  data.push(b);
  data.push(a);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data.top(), c);
  data.pop();
  EXPECT_EQ(data.top(), a);
  data.pop();
  EXPECT_EQ(data.top(), b);
}

TEST(StackTests, DispatchTransferFetch) {
  std::vector<uint8_t> mem{0x11, 0x22, 0x33};
  auto stack = make_stack_with_data(ISA::Operation::FETCH, std::nullopt, mem);
  auto &data = StackTestAccess::data(stack);
  data.push(9);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top(), 0x2211U);
}

} // namespace
