#include "backend/isa/isa.hpp"
#include <algorithm>
#include <backend/vm/stack.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>

namespace {

std::unique_ptr<Cell> make_cell(uint64_t value, bool function = false) {
  return std::make_unique<Cell>(Cell{value, function});
}

std::unique_ptr<Cell> clone_cell(const Cell &cell) {
  return make_cell(cell.value, cell.function);
}

} // namespace

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

MachineState Stack::setState(MachineState next) {
  this->machine_state = next;
  return next;
}

MachineState Stack::run_program() {
  this->machine_state = runInstruction();
  while (this->machine_state == MachineState::OKAY) {
    const auto prev_pc = this->pc;
    if (this->pc == prev_pc) {
      this->pc += 9;
    }
    continue;
  }
  return setState(this->machine_state);
}

MachineState Stack::runInstruction() {
  uint8_t curr = this->program_mem[this->pc];
  const auto spec = ISA::spec_list[curr];
  switch (spec.operation) {
  case (ISA::OperationKind::ARITHMETIC): {
    return setState(this->handleArithmetic(curr, spec));
  }
  case (ISA::OperationKind::CONTROL): {
    return setState(this->handleControl(curr, spec));
  }
  case (ISA::OperationKind::TRANSFER): {
    return setState(this->handleTransfer(curr, spec));
  }
  case (ISA::OperationKind::LOGIC): {
    return setState(this->handleLogic(curr, spec));
  }
  default:
    return setState(MachineState::INVALID_OP);
  }
};

MachineState Stack::handleArithmetic(uint8_t op, ISA::Spec spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::ADD): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value + b->value}));
    break;
  }
  case (ISA::Operation::SUB): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value - b->value}));
    break;
  }
  case (ISA::Operation::MUL): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value * b->value}));
    break;
  }
  case (ISA::Operation::DIV): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value / b->value}));
    break;
  }
  case (ISA::Operation::MOD): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value % b->value}));
    break;
  }
  case (ISA::Operation::INC): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value + 1}));
    break;
  }
  case (ISA::Operation::DEC): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{a->value - 1}));
    break;
  }
  case (ISA::Operation::MAX): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{std::max(a->value, b->value)}));
    break;
  }
  case (ISA::Operation::MIN): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(Cell{std::min(a->value, b->value)}));
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-arithmatic operation dispatched to arithmetic handler");
  }
  return setState(MachineState::OKAY);
};
MachineState Stack::handleLogic(uint8_t op, ISA::Spec spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::LT): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(
        Cell{static_cast<uint64_t>(a->value < b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::LE): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(
        Cell{static_cast<uint64_t>(a->value <= b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::EQ): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(
        Cell{static_cast<uint64_t>(a->value == b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::GE): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(
        Cell{static_cast<uint64_t>(a->value >= b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::GT): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::make_unique<Cell>(
        Cell{static_cast<uint64_t>(a->value > b->value ? 1 : 0)}));
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-logical operation dispatched to logic handler");
  }
  return setState(MachineState::OKAY);
};
MachineState Stack::handleTransfer(uint8_t op, ISA::Spec spec) {
  const uint64_t operand = read_operand(this->program_mem, this->pc);
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::DROP): {
    data_stack.pop();
    break;
  }
  case (ISA::Operation::DUP): {
    data_stack.push(clone_cell(*data_stack.top()));
    break;
  }
  case (ISA::Operation::NDUP): {
    for (auto i = 1; i < operand; i++) {
      data_stack.push(clone_cell(*data_stack.top()));
    }
    break;
  }
  case (ISA::Operation::SWAP): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::move(a));
    data_stack.push(std::move(b));
    break;
  }
  case (ISA::Operation::ROT): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    auto c = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::move(a));
    data_stack.push(std::move(c));
    data_stack.push(std::move(b));
    break;
  }
  case (ISA::Operation::NROT): {
    // stupid implementations are stupid
    auto tmp = std::list<std::unique_ptr<Cell>>();
    // append from stack in decending order
    for (auto i = 0; i < operand; i++) {
      tmp.push_front(std::move(data_stack.top()));
      data_stack.pop();
    }
    // insert the first element (a) into the bottom of the N operations
    data_stack.push(std::move(tmp.front()));
    tmp.pop_front();
    // iterate backwards through the list mainting the rest of the original
    // order
    for (auto i = tmp.size(); i > 0; i--) {
      data_stack.push(std::move(tmp.back()));
      tmp.pop_back();
    }
    break;
  }
  case (ISA::Operation::TUCK): {
    auto a = std::move(data_stack.top());
    data_stack.pop();
    auto b = std::move(data_stack.top());
    data_stack.pop();
    auto c = std::move(data_stack.top());
    data_stack.pop();
    data_stack.push(std::move(b));
    data_stack.push(std::move(a));
    data_stack.push(std::move(c));
    break;
  }
  case (ISA::Operation::NTUCK): {
    auto tmp = std::list<std::unique_ptr<Cell>>();
    // append from stack in decending order
    for (auto i = 0; i < operand; i++) {
      tmp.push_front(std::move(data_stack.top()));
      data_stack.pop();
    }
    // iterate backwards through the list mainting the rest of the original
    for (auto i = tmp.back()->value - 1; i > 0; i--) {
      data_stack.push(std::move(tmp.back()));
      tmp.pop_back();
    }
    // insert the last element (n) into the top of the N operations
    data_stack.push(std::move(tmp.back()));
    break;
  }
  case (ISA::Operation::SIZE): {
    data_stack.push(make_cell(data_stack.size()));
    break;
  }
  case (ISA::Operation::NRND): {
    throw std::invalid_argument("not yet implemented");
    break;
  }
  case (ISA::Operation::PUSH): {
    data_stack.push(make_cell(operand));
    break;
  }
  case (ISA::Operation::FETCH): {
    auto add = static_cast<size_t>(data_stack.top()->value);
    data_stack.pop();
    // fetch getting back the 16 bits is a little strange, not sure if I should
    // switch the WORD of this VM to be 16 or just change FETCH
    uint16_t val = (this->program_mem[add + 1] << 8 | this->program_mem[add]);
    data_stack.push(make_cell(val));
    break;
  }
  default:
    throw std::invalid_argument("non-control operation handed to");
  }
  return setState(MachineState::OKAY);
}
MachineState Stack::handleControl(uint8_t op, ISA::Spec) {
  switch (static_cast<ISA::Operation>(op)) {
  case (ISA::Operation::CALL): {
    // Closure handles are heap indexes. Calling one restores captured values so
    // the callee sees the same left-to-right stack order they had before
    // MKCLOSURE consumed them.
    this->return_stack.push(make_cell(this->pc, true));
    auto heap_idx = this->data_stack.top()->value;
    data_stack.pop();
    CodeEnv *env = &this->heap[heap_idx];
    for (size_t i = env->captured_vars.size(); i > 0; --i) {
      this->data_stack.push(clone_cell(*env->captured_vars[i - 1]));
    }
    this->pc = env->code_idx;
    break;
  }
  case (ISA::Operation::RET): {
    auto dest = std::move(this->return_stack.top());
    this->return_stack.pop();
    this->pc = dest->value;
    break;
  }
  case (ISA::Operation::JMP): {
    auto dest = std::move(this->data_stack.top());
    this->data_stack.pop();
    this->pc = dest->value;
    break;
  }
  case (ISA::Operation::CJMP): {
    // (condition)
    // (location)
    // so for if, the location is evaluated first and then the condition, this
    // works roughly as evaluating the AST from the leaves up
    auto a = std::move(this->data_stack.top());
    this->data_stack.pop();
    auto b = std::move(this->data_stack.top());
    this->data_stack.pop();
    if (a->value != 0) {
      this->pc = b->value;
    }
    break;
  }
  case (ISA::Operation::WAIT): {
    return setState(MachineState::OKAY);
  }
  case (ISA::Operation::HALT): {
    return setState(MachineState::HALT);
  }
  case (ISA::Operation::MKCLOSURE): {
    // Stack contract:
    //   ... captured_1 captured_2 ... captured_n n
    // becomes
    //   ... closure_handle
    // where captured_n is at the top before n is popped. Captures are moved
    // into the heap env and later restored by CALL in the original order.
    CodeEnv ret;
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto n = std::move(this->data_stack.top());
    this->data_stack.pop();
    for (size_t i = 0; i < n->value; i++) {
      auto a = std::move(this->data_stack.top());
      this->data_stack.pop();
      ret.captured_vars.push_back(std::move(a));
    }
    ret.code_idx = operand;
    this->heap.push_back(std::move(ret));
    this->data_stack.push(make_cell(this->heap.size() - 1, true));
    break;
  }
  case (ISA::Operation::MKGLOBAL): {
    // read from the operand the global map label, error check against repeat
    // labels (?)
    // add to the map the current PC code generator should then emit + 1
    // the rhs of the global + RET
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->global_tbl[operand] = make_cell(this->pc + 1, true);
    break;
  }
  case (ISA::Operation::LOADGLOBAL): {
    //  1. read from the operand the global we'd like to retrieve
    //  2. read from global table and copy it to the stack
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->data_stack.push(make_cell(this->global_tbl[operand]->value,
                                    this->global_tbl[operand]->function));
    break;
  }
  case (ISA::Operation::MUTGLOBAL): {
    // Pop a value and store it under the symbol-id operand.
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto value = std::move(this->data_stack.top());
    this->data_stack.pop();
    this->global_tbl[operand] = std::move(value);
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-control operation dispatched to control handler");
  }
  return setState(MachineState::OKAY);
}
