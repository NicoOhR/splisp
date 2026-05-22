#pragma once

#include "frontend/core.hpp"
#include <algorithm>
#include <backend/isa/isa.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
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

struct Cell {
  int64_t value;
  bool function = 0;
};

struct CodeEnv {
  uint64_t code_idx;
  std::vector<std::shared_ptr<Cell>> captured_vars;
};

class Stack {
public:
  Stack(std::vector<ISA::Instruction> program, std::vector<uint8_t> data = {0},
        bool dbg = false);
  // run instruction and handle state
  void advanceProgram();
  MachineState run_program();
  MachineState run_program_dbg(const std::vector<ISA::Instruction> &source);

private:
  friend struct StackTestAccess;
  // dispatches to the handlers
  MachineState runInstruction();
  MachineState setState(MachineState next);
  bool dbg;

  // I am almost certain there is a better way to propagate
  // the machine error that does not throw a program exception
  MachineState handleArithmetic(uint8_t op);
  MachineState handleLogic(uint8_t op);
  MachineState handleTransfer(uint8_t op);
  MachineState handleControl(uint8_t op);

  size_t pc = 0;
  MachineState machine_state = MachineState::OKAY;

  std::map<core::SymbolId, std::shared_ptr<Cell>> global_tbl;
  std::vector<CodeEnv> heap;
  std::size_t frame_base = 0;
  std::stack<std::size_t, std::vector<std::size_t>> frame_base_stack;
  std::vector<uint8_t> program_mem;
  std::vector<std::shared_ptr<Cell>> data_stack;
  std::stack<std::shared_ptr<Cell>> return_stack;
};
