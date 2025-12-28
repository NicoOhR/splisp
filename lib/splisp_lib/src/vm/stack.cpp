#include "isa/isa.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <list>
#include <stdexcept>
#include <vm/stack.hpp>

Stack::Stack(std::vector<ISA::Instruction> program)
    : program_mem(std::move(program)) {};

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
  return MachineState::OKAY;
};
MachineState Stack::handleLogic(ISA::Instruction instr, ISA::Spec spec) {
  switch (instr.op) {
  case (ISA::Operation::LT): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a < b ? 1 : 0);
  }
  case (ISA::Operation::LE): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a <= b ? 1 : 0);
  }
  case (ISA::Operation::EQ): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a == b ? 1 : 0);
  }
  case (ISA::Operation::GE): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a > b ? 1 : 0);
  }
  case (ISA::Operation::GT): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a >= b ? 1 : 0);
  }
  default:
    throw std::invalid_argument(
        "Non-logical operation dispatched to logic handler");
  }
  return MachineState::OKAY;
};
MachineState Stack::handleTransfer(ISA::Instruction instr, ISA::Spec spec) {
  switch (instr.op) {
  case (ISA::Operation::DROP): {
    data_stack.pop();
  }
  case (ISA::Operation::DUP): {
    data_stack.push(data_stack.top());
  }
  case (ISA::Operation::NDUP): {
    for (auto i = 1; i < instr.operand.value(); i++) {
      data_stack.push(data_stack.top());
    }
  }
  case (ISA::Operation::SWAP): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(b);
    data_stack.push(a);
  }
  case (ISA::Operation::ROT): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    auto c = data_stack.top();
    data_stack.pop();
    data_stack.push(a);
    data_stack.push(c);
    data_stack.push(b);
  }
  case (ISA::Operation::NROT): {
    // stupid implementations are stupid
    auto tmp = std::list<uint64_t>();
    // append from stack in decending order
    for (auto i = 0; i < instr.operand.value(); i++) {
      tmp.push_front(data_stack.top());
      data_stack.pop();
    }
    // insert the first element (a) into the bottom of the N operations
    data_stack.push(tmp.front());
    tmp.pop_front();
    // iterate backwards through the list mainting the rest of the original
    // order
    for (auto i = tmp.back(); i > 0; i--) {
      data_stack.push(tmp.back());
      tmp.pop_back();
    }
  }
  case (ISA::Operation::TUCK): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    auto c = data_stack.top();
    data_stack.pop();
    data_stack.push(b);
    data_stack.push(a);
    data_stack.push(c);
  }
  case (ISA::Operation::NTUCK): {
    auto tmp = std::list<uint64_t>();
    // append from stack in decending order
    for (auto i = 0; i < instr.operand.value(); i++) {
      tmp.push_front(data_stack.top());
      data_stack.pop();
    }
    // iterate backwards through the list mainting the rest of the original
    for (auto i = tmp.back() - 1; i > 0; i--) {
      data_stack.push(tmp.back());
      tmp.pop_back();
    }
    // insert the last element (n) into the top of the N operations
    data_stack.push(tmp.back());
  }
  case (ISA::Operation::SIZE): {
    data_stack.push(data_stack.size());
  }
  case (ISA::Operation::NRND): {
    throw std::invalid_argument("not yet implemented");
  }
  case (ISA::Operation::PUSH): {
    data_stack.push(instr.operand.value());
  }
  case (ISA::Operation::FETCH): {
    auto add = data_stack.top();
    data_stack.pop();
    data_stack.push(
        static_cast<uint64_t>(this->program_mem[static_cast<size_t>(add)]));
  }
  }
};
MachineState Stack::handleControl(ISA::Instruction, ISA::Spec){};
