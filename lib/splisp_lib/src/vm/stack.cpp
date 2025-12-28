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
    return this->handleArithmetic(curr, spec);
  }
  case (ISA::OperationKind::CONTROL): {
    return this->handleControl(curr, spec);
  }
  case (ISA::OperationKind::TRANSFER): {
    return this->handleTransfer(curr, spec);
  }
  case (ISA::OperationKind::LOGIC): {
    return this->handleLogic(curr, spec);
  }
  default:
    break;
  }
};

MachineState Stack::handleArithmetic(ISA::Instruction, ISA::Spec){};
MachineState Stack::handleLogic(ISA::Instruction, ISA::Spec){};
MachineState Stack::handleTransfer(ISA::Instruction, ISA::Spec){};
MachineState Stack::handleControl(ISA::Instruction, ISA::Spec){};
