#include "parser.hpp"
#include <iostream>
#include <lexer.hpp>
#include <string>

int main() {
  // std::string program = "(((+ 3 3) (+ 3 4)) 1 2)";
  std::string program = "(if (1 > 2) 1 2)";
  Lexer lex(program);
  Parser parser(lex);
  std::cout << "--+--" << std::endl;
  parser.print_ast();
  return 0;
}
