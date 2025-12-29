#include "isa/isa.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <list>
#include <stdexcept>
#include <vm/stack.hpp>

Stack::Stack(std::vector<ISA::Instruction> program, std::vector<uint8_t> data) {
  std::vector<uint8_t> program_bytes;
  for (auto instr : program) {
    auto tmp = instr.to_bytes();
    program_bytes.insert(program_bytes.end(), tmp.begin(), tmp.end());
  }
  this->program_mem.insert(this->program_mem.end(), program_bytes.begin(),
                           program_bytes.end());
  this->program_mem.insert(this->program_mem.end(), data.begin(), data.end());
};

uint64_t read_operand(const std::vector<uint8_t> &mem, size_t pc) {
  uint64_t value = 0;
  for (size_t i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(mem[pc + 1 + i]) << (8 * i);
  }
  return value;
}

void Stack::advanceProgram() {
  MachineState state = runInstruction();
  if (state == OKAY) {
    this->pc += 9;
  } else {
    throw std::exception();
  }
};

MachineState Stack::runInstruction() {
  uint8_t curr = this->program_mem[this->pc];
  const auto spec = ISA::spec_list[curr];
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
    return MachineState::INVALID_OP;
  }
};

MachineState Stack::handleArithmetic(uint8_t op, ISA::Spec spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::ADD): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a + b);
    break;
  }
  case (ISA::Operation::SUB): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a - b);
    break;
  }
  case (ISA::Operation::MUL): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a * b);
    break;
  }
  case (ISA::Operation::DIV): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a / b);
    break;
  }
  case (ISA::Operation::MOD): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a % b);
    break;
  }
  case (ISA::Operation::INC): {
    auto a = data_stack.top();
    data_stack.pop();
    data_stack.push(a + 1);
    break;
  }
  case (ISA::Operation::DEC): {
    auto a = data_stack.top();
    data_stack.pop();
    data_stack.push(a - 1);
    break;
  }
  case (ISA::Operation::MAX): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(std::max(a, b));
    break;
  }
  case (ISA::Operation::MIN): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(std::min(a, b));
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-arithmatic operation dispatched to arithmetic handler");
  }
  return MachineState::OKAY;
};
MachineState Stack::handleLogic(uint8_t op, ISA::Spec spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::LT): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a < b ? 1 : 0);
    break;
  }
  case (ISA::Operation::LE): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a <= b ? 1 : 0);
    break;
  }
  case (ISA::Operation::EQ): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a == b ? 1 : 0);
    break;
  }
  case (ISA::Operation::GE): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a > b ? 1 : 0);
    break;
  }
  case (ISA::Operation::GT): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(a >= b ? 1 : 0);
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-logical operation dispatched to logic handler");
  }
  return MachineState::OKAY;
};
MachineState Stack::handleTransfer(uint8_t op, ISA::Spec spec) {
  const uint64_t operand = read_operand(this->program_mem, this->pc);
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::DROP): {
    data_stack.pop();
    break;
  }
  case (ISA::Operation::DUP): {
    data_stack.push(data_stack.top());
    break;
  }
  case (ISA::Operation::NDUP): {
    for (auto i = 1; i < operand; i++) {
      data_stack.push(data_stack.top());
    }
    break;
  }
  case (ISA::Operation::SWAP): {
    auto a = data_stack.top();
    data_stack.pop();
    auto b = data_stack.top();
    data_stack.pop();
    data_stack.push(b);
    data_stack.push(a);
    break;
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
    break;
  }
  case (ISA::Operation::NROT): {
    // stupid implementations are stupid
    auto tmp = std::list<uint64_t>();
    // append from stack in decending order
    for (auto i = 0; i < operand; i++) {
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
    break;
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
    break;
  }
  case (ISA::Operation::NTUCK): {
    auto tmp = std::list<uint64_t>();
    // append from stack in decending order
    for (auto i = 0; i < operand; i++) {
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
    break;
  }
  case (ISA::Operation::SIZE): {
    data_stack.push(data_stack.size());
    break;
  }
  case (ISA::Operation::NRND): {
    throw std::invalid_argument("not yet implemented");
    break;
  }
  case (ISA::Operation::PUSH): {
    data_stack.push(operand);
    break;
  }
  case (ISA::Operation::FETCH): {
    auto add = static_cast<size_t>(data_stack.top());
    data_stack.pop();
    // fetch getting back the 16 bits is a little strange, not sure if I should
    // switch the WORD of this VM to be 16 or just change FETCH
    uint16_t val = (this->program_mem[add + 1] << 8 | this->program_mem[add]);
    data_stack.push(val);
    break;
  }
  default:
    throw std::invalid_argument("non-control operation handed to");
  }
  return MachineState::OKAY;
}
MachineState Stack::handleControl(uint8_t op, ISA::Spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::CALL): {
    this->return_stack.push(this->pc);
    auto dest = this->data_stack.top();
    data_stack.pop();
    this->pc = dest;
  }
  case (ISA::Operation::RET): {
    auto dest = this->return_stack.top();
    this->return_stack.pop();
    this->pc = dest;
  }
  case (ISA::Operation::JMP): {
    auto dest = this->data_stack.top();
    this->data_stack.pop();
    this->pc = dest;
  }
  case (ISA::Operation::CJMP): {
    auto a = this->data_stack.top();
    this->data_stack.pop();
    auto b = this->data_stack.top();
    this->data_stack.pop();
    if (a != 0) {
      this->pc = b;
    }
  }
  case (ISA::Operation::WAIT): {
    return MachineState::OKAY;
  }
  case (ISA::Operation::HALT): {
    return MachineState::HALT;
  }
  default:
    throw std::invalid_argument(
        "Non-control operation dispatched to control handler");
  }
}
