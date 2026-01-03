#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <backend/generator/generator.hpp>
#include <backend/vm/stack.hpp>

struct GeneratorTestAccess {
  static void lower_keyword(Generator &gen, const ast::List &list) {
    gen.lower_keyword(list);
  }

  static const std::vector<ISA::Instruction> &program(const Generator &gen) {
    return gen.program;
  }
};

struct StackTestAccess {
  static MachineState runInstruction(Stack &stack) {
    return stack.runInstruction();
  }

  static std::stack<uint64_t> &data(Stack &stack) { return stack.data_stack; }
  static size_t &pc(Stack &stack) { return stack.pc; }
};

constexpr size_t kInstrSize = 9;

std::unique_ptr<ast::SExp> make_keyword(ast::Keyword keyword) {
  auto sexp = std::make_unique<ast::SExp>();
  sexp->node = ast::Symbol{keyword};
  return sexp;
}

std::unique_ptr<ast::SExp> make_bool(bool value) {
  auto sexp = std::make_unique<ast::SExp>();
  sexp->node = ast::Symbol{value};
  return sexp;
}

std::unique_ptr<ast::SExp> make_number(uint64_t value) {
  auto sexp = std::make_unique<ast::SExp>();
  sexp->node = ast::Symbol{value};
  return sexp;
}

TEST(GeneratorTests, IfEmitsJumpTargets) {
  ast::List if_list;
  if_list.list.push_back(make_keyword(ast::Keyword::if_expr));
  if_list.list.push_back(make_bool(true));
  if_list.list.push_back(make_number(42));
  if_list.list.push_back(make_number(99));

  ast::AST empty_ast;
  Generator gen(empty_ast);
  GeneratorTestAccess::lower_keyword(gen, if_list);

  const auto &program = GeneratorTestAccess::program(gen);
  ASSERT_EQ(program.size(), 7U);

  const uint64_t then_index = 6;
  const uint64_t end_index = 7;

  EXPECT_EQ(program[0].op, ISA::Operation::PUSH);
  EXPECT_EQ(program[0].operand, then_index);

  EXPECT_EQ(program[1].op, ISA::Operation::PUSH);
  EXPECT_EQ(program[1].operand, 1U);

  EXPECT_EQ(program[2].op, ISA::Operation::CJMP);
  EXPECT_EQ(program[2].operand, std::nullopt);

  EXPECT_EQ(program[3].op, ISA::Operation::PUSH);
  EXPECT_EQ(program[3].operand, 99U);

  EXPECT_EQ(program[4].op, ISA::Operation::PUSH);
  EXPECT_EQ(program[4].operand, end_index);

  EXPECT_EQ(program[5].op, ISA::Operation::JMP);
  EXPECT_EQ(program[5].operand, std::nullopt);

  EXPECT_EQ(program[6].op, ISA::Operation::PUSH);
  EXPECT_EQ(program[6].operand, 42U);
}

TEST(GeneratorTests, IfRunsOnVm) {
  ast::List if_list;
  if_list.list.push_back(make_keyword(ast::Keyword::if_expr));
  if_list.list.push_back(make_bool(true));
  if_list.list.push_back(make_number(42));
  if_list.list.push_back(make_number(99));

  ast::AST empty_ast;
  Generator gen(empty_ast);
  GeneratorTestAccess::lower_keyword(gen, if_list);

  auto program = GeneratorTestAccess::program(gen);
  program.push_back(ISA::Instruction{.op = ISA::Operation::HALT});

  std::vector<uint8_t> data{};
  Stack stack(std::move(program), std::move(data));
  MachineState state = MachineState::OKAY;
  for (size_t i = 0; i < 100; ++i) {
    const auto prev_pc = StackTestAccess::pc(stack);
    state = StackTestAccess::runInstruction(stack);
    if (state == MachineState::HALT) {
      break;
    }
    if (state != MachineState::OKAY) {
      break;
    }
    if (StackTestAccess::pc(stack) == prev_pc) {
      StackTestAccess::pc(stack) += kInstrSize;
    }
  }

  EXPECT_EQ(state, MachineState::HALT);
  auto &data_stack = StackTestAccess::data(stack);
  ASSERT_EQ(data_stack.size(), 1U);
  EXPECT_EQ(data_stack.top(), 42U);
}
