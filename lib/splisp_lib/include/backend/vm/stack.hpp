#pragma once

#include <backend/isa/isa.hpp>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <vector>

enum MachineState {
  OKAY,
  HALT,
  INVALID_ADD,
  INVALID_INSTR,
  INVALID_OP,
  STACK_OVERFLOW,
  STACK_UNDERFLOW
};

class Stack {
public:
  Stack(std::vector<ISA::Instruction> program, std::vector<uint8_t> data);
  // run instruction and handle state
  void advanceProgram();
  MachineState run_program();

private:
  friend struct StackTestAccess;
  // dispatches to the handlers
  MachineState runInstruction();
  MachineState setState(MachineState next);

  // I am almost certain there is a better way to propagate
  // the machine error that does not throw a program exception
  MachineState handleArithmetic(uint8_t op, ISA::Spec spec);
  MachineState handleLogic(uint8_t op, ISA::Spec spec);
  MachineState handleTransfer(uint8_t op, ISA::Spec spec);
  MachineState handleControl(uint8_t op, ISA::Spec spec);

  size_t pc = 0;
  MachineState machine_state = MachineState::OKAY;
  std::vector<uint8_t> program_mem;
  std::stack<uint64_t> data_stack;
  std::stack<uint64_t> return_stack;
};
