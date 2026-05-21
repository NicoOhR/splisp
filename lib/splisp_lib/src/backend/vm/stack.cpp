#include "backend/isa/isa.hpp"
#include <algorithm>
#include <backend/vm/stack.hpp>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

std::shared_ptr<Cell> make_cell(int64_t value, bool function = false) {
  return std::make_shared<Cell>(Cell{value, function});
}

std::shared_ptr<Cell> clone_cell(const Cell &cell) {
  return make_cell(cell.value, cell.function);
}

} // namespace

Stack::Stack(std::vector<ISA::Instruction> program, std::vector<uint8_t> data,
             bool dbg) {
  std::vector<uint8_t> program_bytes;
  this->dbg = dbg;
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

MachineState
Stack::run_program_dbg(const std::vector<ISA::Instruction> &source) {
  auto print_state = [&]() {
    // program listing with current instruction marked
    std::cerr << "── program ──\n";
    for (size_t i = 0; i < source.size(); i++) {
      const auto &spec = ISA::spec_list[static_cast<uint8_t>(source[i].op)];
      bool current = (i * 9 == this->pc);
      std::cerr << (current ? " * " : "   ") << "[" << i << "] "
                << spec.mnemonic;
      if (source[i].operand.has_value())
        std::cerr << " " << source[i].operand.value();
      std::cerr << "\n";
    }
    // data stack
    std::cerr << "── data stack (bottom→top) ──\n";
    for (size_t i = 0; i < this->data_stack.size(); i++) {
      std::cerr << "  [" << i << "] " << this->data_stack[i]->value
                << (this->data_stack[i]->function ? "f" : "")
                << (i == this->frame_base ? "  ← frame_base" : "") << "\n";
    }
    // return stack
    {
      std::cerr << "── return stack (top first) ──\n";
      auto tmp = this->return_stack;
      while (!tmp.empty()) {
        std::cerr << "  " << tmp.top()->value << "\n";
        tmp.pop();
      }
    }
    // heap
    std::cerr << "── heap ──\n";
    for (size_t i = 0; i < this->heap.size(); i++) {
      std::cerr << "  [" << i << "] code_idx=" << this->heap[i].code_idx
                << " captures=[";
      for (size_t j = 0; j < this->heap[i].captured_vars.size(); j++) {
        if (j)
          std::cerr << ", ";
        std::cerr << this->heap[i].captured_vars[j]->value;
      }
      std::cerr << "]\n";
    }
    // globals
    std::cerr << "── globals ──\n";
    for (auto &[id, cell] : this->global_tbl) {
      std::cerr << "  [" << id << "] " << cell->value
                << (cell->function ? "f" : "") << "\n";
    }
    std::cerr << "────────────  pc=" << this->pc
              << "  frame_base=" << this->frame_base << "\n";
  };

  while (true) {
    print_state();
    std::cerr << "[enter to step, q+enter to quit] ";
    std::string line;
    std::getline(std::cin, line);
    if (line == "q")
      break;

    const auto prev_pc = this->pc;
    try {
      this->machine_state = runInstruction();
    } catch (const std::exception &e) {
      const uint8_t op = this->program_mem[prev_pc];
      std::cerr << "runtime error at pc=" << prev_pc
                << " op=" << ISA::spec_list[op].mnemonic << ": " << e.what()
                << "\n";
      return setState(MachineState::INVALID_OP);
    }
    if (this->machine_state != MachineState::OKAY)
      break;
    if (this->pc == prev_pc)
      this->pc += 9;
    if (this->pc >= this->program_mem.size()) {
      print_state();
      return setState(MachineState::HALT);
    }
  }
  print_state();
  return setState(this->machine_state);
}

MachineState Stack::run_program() {
  while (true) {
    const auto prev_pc = this->pc;
    try {
      this->machine_state = runInstruction();
    } catch (const std::exception &e) {
      const uint8_t op = this->program_mem[prev_pc];
      const auto &spec = ISA::spec_list[op];
      std::cerr << "runtime error at pc=" << prev_pc << " op=" << spec.mnemonic
                << ": " << e.what() << "\n";
      std::cerr << "  stack depth=" << this->data_stack.size()
                << " return_stack depth=" << this->return_stack.size()
                << " heap size=" << this->heap.size() << "\n";
      return setState(MachineState::INVALID_OP);
    }
    if (this->machine_state != MachineState::OKAY)
      break;
    if (this->pc == prev_pc)
      this->pc += 9;
    if (this->pc >= this->program_mem.size())
      return MachineState::HALT;
    continue;
  }
  return setState(this->machine_state);
}

MachineState Stack::runInstruction() {
  uint8_t curr = this->program_mem[this->pc];
  const auto spec = ISA::spec_list[curr];
  if (this->dbg) {
    std::cerr << "pc=" << this->pc << " op=" << spec.mnemonic << " stack=[";
    for (size_t i = 0; i < this->data_stack.size(); ++i) {
      if (i)
        std::cerr << ", ";
      std::cerr << this->data_stack.at(i)->value;
      if (this->data_stack.at(i)->function)
        std::cerr << "f";
    }
    std::cerr << "]\n";
  }
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
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    if (a->function || b->function) {
      return MachineState::INVALID_ADD;
    }
    data_stack.push_back(std::make_shared<Cell>(Cell{a->value + b->value}));
    break;
  }
  case (ISA::Operation::SUB): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    if (a->function || b->function) {
      return MachineState::INVALID_ADD;
    }
    data_stack.push_back(std::make_shared<Cell>(
        Cell{.value = b->value - a->value, .function = false}));
    break;
  }
  case (ISA::Operation::MUL): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    if (a->function || b->function) {
      return MachineState::INVALID_ADD;
    }
    data_stack.push_back(std::make_shared<Cell>(Cell{a->value * b->value}));
    break;
  }
  case (ISA::Operation::DIV): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    if (a->function || b->function) {
      return MachineState::INVALID_ADD;
    }
    data_stack.push_back(std::make_shared<Cell>(Cell{b->value / a->value}));
    break;
  }
  case (ISA::Operation::MOD): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    if (a->function || b->function) {
      return MachineState::INVALID_ADD;
    }
    data_stack.push_back(std::make_shared<Cell>(Cell{b->value % a->value}));
    break;
  }
  case (ISA::Operation::INC): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(Cell{a->value + 1}));
    break;
  }
  case (ISA::Operation::DEC): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(Cell{a->value - 1}));
    break;
  }
  case (ISA::Operation::MAX): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(
        std::make_shared<Cell>(Cell{std::max(a->value, b->value)}));
    break;
  }
  case (ISA::Operation::MIN): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(
        std::make_shared<Cell>(Cell{std::min(a->value, b->value)}));
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
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(
        Cell{static_cast<int64_t>(a->value < b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::LE): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(
        Cell{static_cast<int64_t>(a->value <= b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::EQ): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(
        Cell{static_cast<int64_t>(a->value == b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::GE): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(
        Cell{static_cast<int64_t>(a->value >= b->value ? 1 : 0)}));
    break;
  }
  case (ISA::Operation::GT): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::make_shared<Cell>(
        Cell{static_cast<int64_t>(a->value > b->value ? 1 : 0)}));
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
    for (uint64_t i = 0; i < operand; i++) {
      data_stack.pop_back();
    }
    break;
  }
  case (ISA::Operation::DUP): {
    data_stack.push_back(clone_cell(*data_stack.back()));
    break;
  }
  case (ISA::Operation::SWAP): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    auto b = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.push_back(std::move(a));
    data_stack.push_back(std::move(b));
    break;
  }
  case (ISA::Operation::NROT): {
    auto a = std::move(data_stack.back());
    data_stack.pop_back();
    data_stack.insert(data_stack.end() - operand + 1, std::move(a));
    break;
  }
  case (ISA::Operation::PUSH): {
    data_stack.push_back(make_cell(operand));
    break;
  }
  case (ISA::Operation::PICK): {
    // PICK n: push a copy of the item at depth n (0 = top).
    data_stack.push_back(
        clone_cell(*data_stack.at(data_stack.size() - 1 - operand)));
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
    // MKCLOSURE consumed them. The closure
    const uint64_t arg_count = read_operand(this->program_mem, this->pc);
    const size_t handle_idx = this->data_stack.size() - arg_count - 1;
    const auto heap_idx = this->data_stack.at(handle_idx)->value;
    this->return_stack.push(make_cell(this->pc + 9, false));
    data_stack.erase(this->data_stack.begin() + handle_idx);
    CodeEnv *env = &this->heap.at(heap_idx);
    for (size_t i = 0; i < env->captured_vars.size(); ++i) {
      this->data_stack.push_back(env->captured_vars[i]);
    }
    this->pc = env->code_idx;
    break;
  }
  case (ISA::Operation::RET): {
    if (this->return_stack.empty())
      throw std::runtime_error("return stack underflow");
    auto dest = std::move(this->return_stack.top());
    this->return_stack.pop();
    this->pc = dest->value;
    break;
  }
  case (ISA::Operation::JMP): {
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->pc = operand;
    break;
  }
  case (ISA::Operation::CJMP): {
    // (condition)
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto a = std::move(this->data_stack.back());
    this->data_stack.pop_back();
    if (a->value != 0) {
      this->pc = operand;
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
    // take as operand the code index which the code env lives in
    // capture everything in the current frame by taking the shared pointer and
    // adding it to the code env
    CodeEnv ret;
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    for (size_t i = this->frame_base; i < this->data_stack.size(); i++) {
      ret.captured_vars.push_back(this->data_stack.at(i));
    }
    ret.code_idx = operand;
    this->heap.push_back(std::move(ret));
    this->data_stack.push_back(make_cell(this->heap.size() - 1, true));
    break;
  }
  case (ISA::Operation::MKGLOBAL): {
    // read from the operand the global map label, error check against repeat
    // labels (?)
    // add to the map the current PC code generator should then emit + 1
    // the rhs of the global + RET
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto glob_value = std::move(this->data_stack.back());
    this->data_stack.pop_back();
    this->global_tbl[operand] = glob_value;
    break;
  }
  case (ISA::Operation::LOADGLOBAL): {
    //  1. read from the operand the global we'd like to retrieve
    //  2. read from global table and copy it to the stack
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->data_stack.push_back(make_cell(this->global_tbl[operand]->value,
                                         this->global_tbl[operand]->function));
    break;
  }
  case (ISA::Operation::MUTGLOBAL): {
    // Pop a value and store it under the symbol-id operand.
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto value = std::move(this->data_stack.back());
    this->data_stack.pop_back();
    this->global_tbl[operand] = std::move(value);
    break;
  }
  case (ISA::Operation::ENTER): {
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->frame_base = this->data_stack.size() - operand;
    break;
  }
  case (ISA::Operation::GETLOCAL): {
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    this->data_stack.push_back(
        clone_cell(*this->data_stack.at(this->frame_base + operand)));
    break;
  }
  case (ISA::Operation::SETLOCAL): {
    // Take as operand the index in the local frame of the variable we'd like to
    // mutate take the value off the top of the stack and mutate the value of
    // the cell at the target index but maintain the pointer, every environment
    // which sees this value will have the mutated value in it
    const uint64_t operand = read_operand(this->program_mem, this->pc);
    auto value = std::move(data_stack.back());
    *data_stack.at(frame_base + operand) = *value;
    data_stack.pop_back();
    break;
  }
  default:
    throw std::invalid_argument(
        "Non-control operation dispatched to control handler");
  }
  return setState(MachineState::OKAY);
}
