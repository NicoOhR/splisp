#pragma once

#include <cstddef>
#include <cstdint>
#include <isa/isa.hpp>
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
  Stack(std::vector<ISA::Instruction> program);
  // run instruction and handle state
  void advanceProgram();

private:
  // dispatches to the handlers
  MachineState runInstruction();

  // I am almost certain there is a better way to proliferate
  // the machine error that does not throw a program expection
  MachineState handleArithmetic(ISA::Instruction, ISA::Spec);
  MachineState handleLogic(ISA::Instruction, ISA::Spec);
  MachineState handleTransfer(ISA::Instruction, ISA::Spec);
  MachineState handleControl(ISA::Instruction, ISA::Spec);

  size_t pc = 0;
  std::vector<ISA::Instruction> program_mem;
  std::stack<uint64_t> data_stack;
  std::stack<uint64_t> return_stack;
};
