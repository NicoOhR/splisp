#include "isa/isa.hpp"
#include <vm/stack.hpp>

Stack::Stack(std::vector<ISA::Instruction> program) : program_mem(program) {};
