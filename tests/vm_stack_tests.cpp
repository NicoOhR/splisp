#include <cstdint>
#include <optional>
#include <stack>
#include <vector>

#include <gtest/gtest.h>

#include <isa/isa.hpp>
#include <vm/stack.hpp>

struct StackTestAccess {
  static MachineState runInstruction(Stack &stack) {
    return stack.runInstruction();
  }

  static std::stack<uint64_t> &data(Stack &stack) { return stack.data_stack; }
};

namespace {

Stack make_stack(ISA::Operation op, std::optional<uint64_t> operand = {}) {
  ISA::Instruction instr{op, operand};
  std::vector<ISA::Instruction> program{instr};
  std::vector<uint8_t> data{};
  return Stack(std::move(program), std::move(data));
}

Stack make_stack_with_data(ISA::Operation op,
                           std::optional<uint64_t> operand,
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

TEST(StackTests, DispatchTransferRot) {
  auto stack = make_stack(ISA::Operation::ROT);
  auto &data = StackTestAccess::data(stack);
  data.push(1);
  data.push(2);
  data.push(3);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data.top(), 1U);
  data.pop();
  EXPECT_EQ(data.top(), 3U);
  data.pop();
  EXPECT_EQ(data.top(), 2U);
}

TEST(StackTests, DispatchTransferTuck) {
  auto stack = make_stack(ISA::Operation::TUCK);
  auto &data = StackTestAccess::data(stack);
  data.push(7);
  data.push(10);
  data.push(20);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 4U);
  EXPECT_EQ(data.top(), 20U);
  data.pop();
  EXPECT_EQ(data.top(), 10U);
  data.pop();
  EXPECT_EQ(data.top(), 20U);
  data.pop();
  EXPECT_EQ(data.top(), 7U);
}

TEST(StackTests, DispatchTransferFetch) {
  std::vector<uint8_t> mem{0x11, 0x22, 0x33};
  auto stack =
      make_stack_with_data(ISA::Operation::FETCH, std::nullopt, mem);
  auto &data = StackTestAccess::data(stack);
  data.push(10);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.top(), 0x22U);
}

} // namespace
