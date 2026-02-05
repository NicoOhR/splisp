#include <frontend/core.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <frontend/scoper.hpp>
#include <iostream>
#include <string>

int main() {
  // std::string program = "(define (add x y) (+ x y) (+ x 1)) (add 2 3) (add 1
  // "
  //                       "2) (define x 2) (add x 3)";
  std::string program = "(define x 2) (define (top y) (+ x y)) (top 3)";
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
  return 0;
}
