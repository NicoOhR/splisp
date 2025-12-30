#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
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
  static size_t &pc(Stack &stack) { return stack.pc; }
};

constexpr size_t kInstrSize = 9;

std::string stack_to_string(const std::stack<uint64_t> &stack) {
  std::ostringstream out;
  auto copy = stack;
  out << "[";
  bool first = true;
  while (!copy.empty()) {
    if (!first) {
      out << ", ";
    }
    first = false;
    out << copy.top();
    copy.pop();
  }
  out << "]";
  return out.str();
}

TEST(StackProgramTests, SimpleAddHalts) {
  std::vector<ISA::Instruction> program{
      {ISA::Operation::PUSH, 2},
      {ISA::Operation::PUSH, 3},
      {ISA::Operation::ADD, std::nullopt},
      {ISA::Operation::HALT, std::nullopt},
  };

  std::vector<uint8_t> data{};
  Stack stack(std::move(program), std::move(data));
  stack.advanceProgram();
  // EXPECT_EQ(state, MachineState::HALT);

  auto &data_stack = StackTestAccess::data(stack);
  ASSERT_EQ(data_stack.size(), 1U);
  EXPECT_EQ(data_stack.top(), 5U);
}

TEST(StackProgramTests, ConditionalJump) {
  constexpr size_t kTrue = 8 * kInstrSize;

  const std::vector<ISA::Instruction> program{
      {ISA::Operation::PUSH, 3},            // 3
      {ISA::Operation::PUSH, 2},            // 2 3
      {ISA::Operation::LT, std::nullopt},   // 1
      {ISA::Operation::PUSH, kTrue},        // 72 1
      {ISA::Operation::SWAP, std::nullopt}, // 1 72
      {ISA::Operation::CJMP, std::nullopt}, // true
      {ISA::Operation::PUSH, 0},            // 0
      {ISA::Operation::HALT, std::nullopt},
      // true:
      {ISA::Operation::PUSH, 1},
      {ISA::Operation::HALT, std::nullopt},
  };

  std::vector<uint8_t> data{};
  Stack stack(std::move(program), std::move(data));
  MachineState state = MachineState::OKAY;
  for (size_t i = 0; i < 100; ++i) {
    const auto prev_pc = StackTestAccess::pc(stack);
    state = StackTestAccess::runInstruction(stack);
    std::cerr << "step " << i << " pc=" << prev_pc
              << " stack=" << stack_to_string(StackTestAccess::data(stack))
              << "\n";
    if (state == MachineState::HALT) {
      break;
    }
    if (state != MachineState::OKAY) {
      break;
    }
    if (StackTestAccess::pc(stack) == prev_pc) {
      StackTestAccess::pc(stack) += kInstrSize;
    }
  }
  EXPECT_EQ(state, MachineState::HALT);

  auto &data_stack = StackTestAccess::data(stack);
  ASSERT_EQ(data_stack.size(), 1U);
  EXPECT_EQ(data_stack.top(), 1U);
}
