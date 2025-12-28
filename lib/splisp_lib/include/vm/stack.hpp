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

  // I am almost certain there is a better way to propagate
  // the machine error that does not throw a program exception
  MachineState handleArithmetic(ISA::Instruction instr, ISA::Spec spec);
  MachineState handleLogic(ISA::Instruction instr, ISA::Spec spec);
  MachineState handleTransfer(ISA::Instruction instr, ISA::Spec spec);
  MachineState handleControl(ISA::Instruction instr, ISA::Spec spec);

  size_t pc = 0;
  std::vector<ISA::Instruction> program_mem;
  std::stack<uint64_t> data_stack;
  std::stack<uint64_t> return_stack;
};
