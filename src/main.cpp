#include <backend/generator/generator.hpp>
#include <backend/vm/stack.hpp>
#include <frontend/core.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/scoper.hpp>
#include <iostream>
#include <string>

int main() {
  std::string program =
      R"(
    (if (null? (car (cons nil 1))) 2 3)
  )";
  Lexer lex(program);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  std::cout << "--+--" << std::endl;
  ast::print_ast(ast);
  std::cout << std::endl << "--+--" << std::endl;
  Scoper scoper;
  scoper.run(ast);
  scoper.resolve(ast);
  ast::print_ast(ast);
  std::cout << std::endl << "--+--" << std::endl;
  core::Lowerer lowerer;
  const core::Program &program_ir = lowerer.lower(ast);
  core::print_program(program_ir);
  Generator gen(program_ir);
  auto bc = gen.generate();
  std::cout << std::endl << "--+--" << std::endl;
  print_bytecode(bc);
  Stack vm(bc, true);
  vm.run_program();
  // vm.run_program_dbg(bc);
  return 0;
}
