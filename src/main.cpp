#include "parser.hpp"
#include <iostream>
#include <lexer.hpp>
#include <string>

int main() {
  std::string program = "(if #t (+ 3 4) 5)";
  Lexer lex(program);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  std::cout << "--+--" << std::endl;
  ast::print_ast(ast);
  return 0;
}
