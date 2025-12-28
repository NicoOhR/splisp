#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
namespace ISA {
enum class Operation : uint8_t {
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  INC,
  DEC,
  MAX,
  MIN,
  LT,
  LE,
  EQ,
  GE,
  GT,
  DROP,
  DUP,
  NDUP,
  SWAP,
  ROT,
  NROT,
  TUCK,
  NTUCK,
  SIZE,
  NRND,
  PUSH,
  FETCH,
  CALL,
  RET,
  JMP,
  CJMP,
  WAIT,
  HALT
};

enum class OperandKind { NONE, U64, ADD };
enum class OperationKind {
  ARITHMETIC,
  LOGIC,
  TRANSFER,
  CONTROL,
};

// information about each operation is described by the Spec struct and stored
// in the spec_table
struct Spec {
  std::string mnemonic;
  OperandKind operand;
  OperationKind operation;
  std::size_t pops;
  std::size_t pushes;
};
constexpr std::size_t op_count = static_cast<std::size_t>(Operation::HALT) + 1;
inline constexpr std::array<Spec, op_count> spec_list{{
    {"add", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"sub", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"mul", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"div", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"mod", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"inc", OperandKind::NONE, OperationKind::ARITHMETIC, 1, 1},
    {"dec", OperandKind::NONE, OperationKind::ARITHMETIC, 1, 1},
    {"max", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"min", OperandKind::NONE, OperationKind::ARITHMETIC, 2, 1},
    {"lt", OperandKind::NONE, OperationKind::LOGIC, 2, 1},
    {"le", OperandKind::NONE, OperationKind::LOGIC, 2, 1},
    {"eq", OperandKind::NONE, OperationKind::LOGIC, 2, 1},
    {"ge", OperandKind::NONE, OperationKind::LOGIC, 2, 1},
    {"gt", OperandKind::NONE, OperationKind::LOGIC, 2, 1},
    {"drop", OperandKind::NONE, OperationKind::TRANSFER, 1, 0},
    {"dup", OperandKind::NONE, OperationKind::TRANSFER, 1, 2},
    // NDUP/NROT/NTUCK have variable stack effects based on the operand.
    {"ndup", OperandKind::U64, OperationKind::TRANSFER, 0, 0},
    {"swap", OperandKind::NONE, OperationKind::TRANSFER, 2, 2},
    {"rot", OperandKind::NONE, OperationKind::TRANSFER, 3, 3},
    {"nrot", OperandKind::U64, OperationKind::TRANSFER, 0, 0},
    {"tuck", OperandKind::NONE, OperationKind::TRANSFER, 2, 3},
    {"ntuck", OperandKind::U64, OperationKind::TRANSFER, 0, 0},
    {"size", OperandKind::NONE, OperationKind::TRANSFER, 0, 1},
    {"nrnd", OperandKind::U64, OperationKind::TRANSFER, 0, 1},
    {"push", OperandKind::U64, OperationKind::TRANSFER, 0, 1},
    {"fetch", OperandKind::ADD, OperationKind::TRANSFER, 0, 1},
    {"call", OperandKind::ADD, OperationKind::CONTROL, 0, 0},
    {"ret", OperandKind::NONE, OperationKind::CONTROL, 0, 0},
    {"jmp", OperandKind::ADD, OperationKind::CONTROL, 0, 0},
    {"cjmp", OperandKind::ADD, OperationKind::CONTROL, 1, 0},
    {"wait", OperandKind::NONE, OperationKind::CONTROL, 0, 0},
    {"halt", OperandKind::NONE, OperationKind::CONTROL, 0, 0},
}};

struct Instruction {
  Operation op;
  std::optional<uint64_t> operand;
};
} // namespace ISA
