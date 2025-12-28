#include "isa/isa.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
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

MachineState Stack::handleArithmetic(ISA::Instruction instr, ISA::Spec spec) {
  switch (instr.op) {
  case (ISA::Operation::ADD): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a + b);
  }
  case (ISA::Operation::SUB): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a - b);
  }
  case (ISA::Operation::MUL): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a * b);
  }
  case (ISA::Operation::DIV): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a / b);
  }
  case (ISA::Operation::MOD): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a % b);
  }
  case (ISA::Operation::INC): {
    auto a = data_stack.top();
    data_stack.pop();
    data_stack.push(a + 1);
  }
  case (ISA::Operation::DEC): {
    auto a = data_stack.top();
    data_stack.pop();
    data_stack.push(a - 1);
  }
  case (ISA::Operation::MAX): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(std::max(a, b));
  }
  case (ISA::Operation::MIN): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(std::min(a, b));
  }
  default:
    throw std::invalid_argument(
        "Non-arithmatic operation dispatched to arithmetic handler");
  }
};
MachineState Stack::handleLogic(ISA::Instruction, ISA::Spec){};
MachineState Stack::handleTransfer(ISA::Instruction, ISA::Spec){};
MachineState Stack::handleControl(ISA::Instruction, ISA::Spec){};
