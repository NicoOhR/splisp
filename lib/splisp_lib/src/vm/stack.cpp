#include "isa/isa.hpp"
#include <cstddef>
#include <exception>
#include <vm/stack.hpp>

Stack::Stack(std::vector<ISA::Instruction> program) : program_mem(program) {};

void Stack::advanceProgram() {
  MachineState state = runInstruction();
  if (state == OKAY) {
    this->pc += 1;
  } else {
    throw std::exception();
  }
};

MachineState Stack::runInstruction() {
  ISA::Instruction curr = this->program_mem[this->pc];
  auto op_idx = static_cast<size_t>(curr.op);
  const auto spec = ISA::spec_list[op_idx];
  switch (spec.operation) {
  case (ISA::OperationKind::ARITHMETIC): {
    return this->handleArithmetic(curr);
  }
  case (ISA::OperationKind::CONTROL): {
    return this->handleControl(curr);
  }
  case (ISA::OperationKind::TRANSFER): {
    return this->handleTransfer(curr);
  }
  case (ISA::OperationKind::LOGIC): {
    return this->handleLogic(curr);
  }
  default:
    break;
  }
};

MachineState Stack::handleArithmetic(ISA::Instruction){};
MachineState Stack::handleLogic(ISA::Instruction){};
MachineState Stack::handleTransfer(ISA::Instruction){};
MachineState Stack::handleControl(ISA::Instruction){};
