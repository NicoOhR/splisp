#include <cstdint>
#include <isa/isa.hpp>

std::array<uint8_t, 9> ISA::Instruction::to_bytes() const {
  std::array<uint8_t, 9> ret{};
  ret[0] = static_cast<uint8_t>(op);

  const uint64_t value = operand.value_or(0);
  ret[1] = static_cast<uint8_t>(value & 0xFF);
  ret[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
  ret[3] = static_cast<uint8_t>((value >> 16) & 0xFF);
  ret[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
  ret[5] = static_cast<uint8_t>((value >> 32) & 0xFF);
  ret[6] = static_cast<uint8_t>((value >> 40) & 0xFF);
  ret[7] = static_cast<uint8_t>((value >> 48) & 0xFF);
  ret[8] = static_cast<uint8_t>((value >> 56) & 0xFF);

  return ret;
}
