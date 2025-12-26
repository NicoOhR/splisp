#include "parser.hpp"
#include <iostream>
#include <lexer.hpp>
#include <string>

int main() {
  std::string program = "(if #t (+ 3 4) 5)";
  Lexer lex(program);
  Parser parser(lex);
  std::cout << "--+--" << std::endl;
  parser.print_ast();
  return 0;
}
