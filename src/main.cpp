#include <frontend/core.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <iostream>
#include <string>

int main() {
  std::string program = "(lambda (x y) (+ x y) (+ x 1))";
  Lexer lex(program);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  std::cout << "--+--" << std::endl;
  ast::print_ast(ast);
  std::cout << std::endl << "--+--" << std::endl;
  core::Lowerer lowerer;
  const core::Program &program_ir = lowerer.lower(ast);
  core::print_program(program_ir);
  return 0;
}
