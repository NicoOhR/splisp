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
  MachineState runInstruction(ISA::Instruction);

private:
  std::vector<ISA::Instruction> program_mem;
  size_t pc = 0;
  std::stack<uint64_t> data_stack;
  std::stack<uint64_t> return_stack;
};
