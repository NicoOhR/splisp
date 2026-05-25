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

  static std::vector<std::shared_ptr<Cell>> &data(Stack &stack) {
    return stack.data_stack;
  }
  static std::stack<std::shared_ptr<Cell>> &returns(Stack &stack) {
    return stack.return_stack;
  }
  static std::map<core::SymbolId, std::shared_ptr<Cell>> &
  globals(Stack &stack) {
    return stack.global_tbl;
  }
  static size_t &pc(Stack &stack) { return stack.pc; }
  static size_t &frame_base(Stack &stack) { return stack.frame_base; }
  static std::stack<std::size_t, std::vector<std::size_t>> &
  frame_base_stack(Stack &stack) {
    return stack.frame_base_stack;
  }
  static std::vector<HeapObject> &heap(Stack &stack) { return stack.heap; }
};

namespace {

constexpr size_t kInstrSize = 9;

Stack make_stack(ISA::Operation op, std::optional<uint64_t> operand = {}) {
  ISA::Instruction instr{op, operand};
  std::vector<ISA::Instruction> program{instr};
  return Stack(std::move(program));
}

TEST(StackTests, DispatchArithmeticAdd) {
  auto stack = make_stack(ISA::Operation::ADD);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{2}));
  data.push_back(std::make_shared<Cell>(Cell{3}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.back()->value, 5U);
}

TEST(StackTests, DispatchArithmeticOpsCoarse) {
  auto run_binary = [](ISA::Operation op, int64_t b, int64_t a) -> int64_t {
    auto stack = make_stack(op);
    auto &data = StackTestAccess::data(stack);
    data.push_back(std::make_shared<Cell>(Cell{b}));
    data.push_back(std::make_shared<Cell>(Cell{a}));
    auto state = StackTestAccess::runInstruction(stack);
    // gtest functionality is not available inside lambdas
    return data.back()->value;
  };

  EXPECT_EQ(run_binary(ISA::Operation::ADD, 3, 2), 5U);
  EXPECT_EQ(run_binary(ISA::Operation::SUB, 10, 3), 7U);
  EXPECT_EQ(run_binary(ISA::Operation::MUL, 6, 7), 42U);
  EXPECT_EQ(run_binary(ISA::Operation::DIV, 20, 4), 5U);
  EXPECT_EQ(run_binary(ISA::Operation::MOD, 20, 7), 6U);
}

TEST(StackTests, DispatchLogicLt) {
  auto stack = make_stack(ISA::Operation::LT);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{2}));
  data.push_back(std::make_shared<Cell>(Cell{1}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.back()->value, 1U);
}

TEST(StackTests, DispatchTransferPush) {
  auto stack = make_stack(ISA::Operation::PUSH, 42);
  auto &data = StackTestAccess::data(stack);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.back()->value, 42U);
}

TEST(StackTests, DispatchControlHalt) {
  auto stack = make_stack(ISA::Operation::HALT);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::HALT);
}

TEST(StackTests, DispatchControlWait) {
  auto stack = make_stack(ISA::Operation::WAIT);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{5}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(data.size(), 1U);
}

TEST(StackTests, DispatchControlJmp) {
  auto stack = make_stack(ISA::Operation::JMP, 7U);
  auto &data = StackTestAccess::data(stack);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 7U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpTaken) {
  auto stack = make_stack(ISA::Operation::CJMP, 9U);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{1})); // condition = true

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 9U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlCjmpNotTaken) {
  auto stack = make_stack(ISA::Operation::CJMP, 9U);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{0})); // condition = false

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 0U);
  EXPECT_EQ(data.size(), 0U);
}

TEST(StackTests, DispatchControlRet) {
  auto stack = make_stack(ISA::Operation::RET);
  auto &returns = StackTestAccess::returns(stack);
  returns.push(std::make_shared<Cell>(Cell{4}));
  // RET also restores frame_base from frame_base_stack (paired with ENTER).
  // Push a placeholder so the test can exercise return-address restoration
  // in isolation without triggering UB on an empty stack.
  StackTestAccess::frame_base_stack(stack).push(0);

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 4U);
  EXPECT_EQ(returns.size(), 0U);
}

TEST(StackTests, DispatchControlMkClosureCapturesFrameSlotsBySharing) {
  // MKCLOSURE should share the same Cell objects from the frame — not clone
  // them. Verified by checking pointer identity between captured_vars and
  // original slots.
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 2},
      {ISA::Operation::MKCLOSURE, 999},
  };
  Stack stack(std::move(program), true);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{4}));
  data.push_back(std::make_shared<Cell>(Cell{6}));

  const Cell *ptr0 = data[0].get();
  const Cell *ptr1 = data[1].get();

  StackTestAccess::runInstruction(stack); // ENTER 2: frame_base = 0
  EXPECT_EQ(StackTestAccess::frame_base(stack), 0U);

  StackTestAccess::pc(stack) += kInstrSize;
  auto state = StackTestAccess::runInstruction(stack); // MKCLOSURE 999
  EXPECT_EQ(state, MachineState::OKAY);

  auto &heap = StackTestAccess::heap(stack);
  ASSERT_EQ(heap.size(), 1U);
  auto &env0 = std::get<CodeEnv>(heap[0]);
  EXPECT_EQ(env0.code_idx, 999U);
  ASSERT_EQ(env0.captured_vars.size(), 2U);
  EXPECT_EQ(env0.captured_vars[0].get(), ptr0); // shared, not cloned
  EXPECT_EQ(env0.captured_vars[1].get(), ptr1);
}

TEST(StackTests, DispatchControlMkClosureCallRestoresSharedCapturesInOrder) {
  // CALL should restore captured_vars in left-to-right order (slot 0 deepest).
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 2},
      {ISA::Operation::MKCLOSURE, 123},
      {ISA::Operation::CALL, 0},
  };
  Stack stack(std::move(program));
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{4}));
  data.push_back(std::make_shared<Cell>(Cell{6}));

  const Cell *ptr0 = data[0].get();
  const Cell *ptr1 = data[1].get();

  StackTestAccess::runInstruction(stack); // ENTER 2
  StackTestAccess::pc(stack) += kInstrSize;
  StackTestAccess::runInstruction(stack); // MKCLOSURE 123: handle on top

  EXPECT_TRUE(data.back()->function); // handle is marked as function
  StackTestAccess::pc(stack) += kInstrSize;
  auto state = StackTestAccess::runInstruction(stack); // CALL
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack), 123U);

  // captures restored in original order: slot 0 deeper, slot 1 on top
  ASSERT_GE(data.size(), 2U);
  EXPECT_EQ(data[data.size() - 2].get(), ptr0);
  EXPECT_EQ(data[data.size() - 1].get(), ptr1);
}

TEST(StackTests, DispatchControlMutGlobalStoresPoppedValueByOperand) {
  auto stack = make_stack(ISA::Operation::MUTGLOBAL, 77);
  auto &data = StackTestAccess::data(stack);
  auto &globals = StackTestAccess::globals(stack);
  data.push_back(std::make_shared<Cell>(Cell{42}));

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
  globals[77] = std::make_shared<Cell>(Cell{42, false});

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  EXPECT_EQ(data.back()->value, 42U);
  EXPECT_FALSE(data.back()->function);
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
  globals[77] = std::make_shared<Cell>(Cell{42, true});
  const auto *stored_ptr = globals[77].get();

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 1U);
  ASSERT_NE(data.back().get(), nullptr);
  EXPECT_EQ(data.back()->value, 42U);
  EXPECT_TRUE(data.back()->function);
  ASSERT_NE(globals[77], nullptr);
  EXPECT_EQ(globals[77].get(), stored_ptr);
  EXPECT_EQ(globals[77]->value, 42U);
  EXPECT_TRUE(globals[77]->function);
}

TEST(StackTests, DispatchControlEnterSetsFrameBase) {
  auto stack = make_stack(ISA::Operation::ENTER, 2);
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{10}));
  data.push_back(std::make_shared<Cell>(Cell{20}));
  data.push_back(std::make_shared<Cell>(Cell{30}));

  auto state = StackTestAccess::runInstruction(stack);
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(data.size(), 3U);
  EXPECT_EQ(StackTestAccess::frame_base(stack), 1U); // size(3) - n(2) = 1
}

TEST(StackTests, DispatchControlGetLocalPushesCloneAtFrameOffset) {
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 3},
      {ISA::Operation::GETLOCAL, 1},
  };
  Stack stack(std::move(program));

  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{10}));
  data.push_back(std::make_shared<Cell>(Cell{20}));
  data.push_back(std::make_shared<Cell>(Cell{30}));

  auto state =
      StackTestAccess::runInstruction(stack); // ENTER 3: frame_base = 0
  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::frame_base(stack), 0U);

  const auto *slot1_ptr = data[1].get();
  StackTestAccess::pc(stack) += kInstrSize;
  state = StackTestAccess::runInstruction(
      stack); // GET_LOCAL 1: push clone of data[0+1]
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 4U);
  EXPECT_EQ(data.back()->value, 20U);
  EXPECT_NE(data.back().get(), slot1_ptr); // cloned, not the same pointer
}

TEST(StackTests, DispatchControlSetLocalMutatesInPlace) {
  // SETLOCAL should update the value of the existing Cell object, not replace
  // the shared_ptr. Verified by checking pointer identity before and after.
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 3},
      {ISA::Operation::SETLOCAL, 1},
  };
  Stack stack(std::move(program));

  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{10}));
  data.push_back(std::make_shared<Cell>(Cell{20}));
  data.push_back(std::make_shared<Cell>(Cell{30}));

  auto state =
      StackTestAccess::runInstruction(stack); // ENTER 3: frame_base = 0
  EXPECT_EQ(state, MachineState::OKAY);

  const Cell *slot1_ptr = data[1].get(); // snapshot before mutation
  data.push_back(std::make_shared<Cell>(Cell{99}));
  StackTestAccess::pc(stack) += kInstrSize;
  state = StackTestAccess::runInstruction(stack); // SET_LOCAL 1
  EXPECT_EQ(state, MachineState::OKAY);
  ASSERT_EQ(data.size(), 3U);
  EXPECT_EQ(data[0]->value, 10U);
  EXPECT_EQ(data[1]->value, 99U);
  EXPECT_EQ(data[2]->value, 30U);
  EXPECT_EQ(data[1].get(), slot1_ptr); // same Cell object, mutated in place
}

TEST(StackTests, DispatchControlCallPopsHandleAndPushesReturnAddress) {
  // CALL must pop the closure handle from data_stack and push the caller's PC
  // onto the return stack.
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 0},
      {ISA::Operation::MKCLOSURE, 77},
      {ISA::Operation::CALL, 0},
  };
  Stack stack(std::move(program));
  auto &data = StackTestAccess::data(stack);
  auto &returns = StackTestAccess::returns(stack);

  StackTestAccess::runInstruction(stack); // ENTER 0
  StackTestAccess::pc(stack) += kInstrSize;
  StackTestAccess::runInstruction(stack); // MKCLOSURE 77
  ASSERT_EQ(data.size(), 1U);
  EXPECT_TRUE(data.back()->function);

  StackTestAccess::pc(stack) += kInstrSize;
  const size_t expected_ret = StackTestAccess::pc(stack) + kInstrSize; // pc+9
  auto state = StackTestAccess::runInstruction(stack);                 // CALL

  EXPECT_EQ(state, MachineState::OKAY);
  EXPECT_TRUE(data.empty()); // handle was popped, no captures to restore
  ASSERT_EQ(returns.size(), 1U);
  EXPECT_EQ(returns.top()->value,
            expected_ret); // return address is instruction after CALL
}

TEST(StackTests, DispatchControlCallRetRoundtrip) {
  // CALL saves pc+9 (the instruction after CALL) on the return stack.
  // RET restores directly to that address, no run_program advance needed.
  //
  // Layout (each instruction = 9 bytes):
  //   byte  0: ENTER 0
  //   byte  9: MKCLOSURE 36
  //   byte 18: CALL          ← saves 27 (= 18+9) to return stack
  //   byte 27: HALT          ← return address lands here
  //   byte 36: ENTER 0       ← body
  //   byte 45: RET
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 0}, {ISA::Operation::MKCLOSURE, 36},
      {ISA::Operation::CALL, 0},  {ISA::Operation::HALT, std::nullopt},
      {ISA::Operation::ENTER, 0}, {ISA::Operation::RET, std::nullopt},
  };
  Stack stack(std::move(program));
  auto &returns = StackTestAccess::returns(stack);

  StackTestAccess::runInstruction(stack); // ENTER 0 at byte 0
  StackTestAccess::pc(stack) += kInstrSize;
  StackTestAccess::runInstruction(stack); // MKCLOSURE 36 at byte 9
  StackTestAccess::pc(stack) += kInstrSize;

  StackTestAccess::runInstruction(stack); // CALL at byte 18 → pc jumps to 36
  EXPECT_EQ(StackTestAccess::pc(stack), 36U);
  ASSERT_EQ(returns.size(), 1U);
  EXPECT_EQ(returns.top()->value,
            27U); // return address = instruction after CALL

  StackTestAccess::runInstruction(stack); // ENTER 0 at byte 36
  StackTestAccess::pc(stack) += kInstrSize;
  auto ret_state = StackTestAccess::runInstruction(stack); // RET at byte 45
  EXPECT_EQ(ret_state, MachineState::OKAY);
  EXPECT_EQ(StackTestAccess::pc(stack),
            27U); // restored to instruction after CALL
  EXPECT_EQ(returns.size(), 0U);
}

TEST(StackTests, DispatchControlSetLocalMutationVisibleThroughSharedCapture) {
  // The key letrec property: a Cell captured by MKCLOSURE and then mutated via
  // SET_LOCAL should reflect the new value through the capture.
  std::vector<ISA::Instruction> program{
      {ISA::Operation::ENTER, 1},
      {ISA::Operation::MKCLOSURE, 999},
      {ISA::Operation::SETLOCAL, 0},
  };
  Stack stack(std::move(program));
  auto &data = StackTestAccess::data(stack);
  data.push_back(std::make_shared<Cell>(Cell{0})); // undef placeholder

  StackTestAccess::runInstruction(stack); // ENTER 1: frame_base = 0
  StackTestAccess::pc(stack) += kInstrSize;

  StackTestAccess::runInstruction(
      stack); // MKCLOSURE 999: captures slot 0 by sharing
  auto &heap = StackTestAccess::heap(stack);
  ASSERT_EQ(heap.size(), 1U);
  auto &env0 = std::get<CodeEnv>(heap[0]);
  EXPECT_EQ(env0.captured_vars[0]->value, 0U); // still undef at capture time
  StackTestAccess::pc(stack) += kInstrSize;

  data.push_back(std::make_shared<Cell>(Cell{42}));
  StackTestAccess::runInstruction(
      stack); // SET_LOCAL 0: mutates slot 0 in place

  // The closure's captured Cell sees the mutation — this is what enables letrec
  EXPECT_EQ(env0.captured_vars[0]->value, 42U);
}

} // namespace
