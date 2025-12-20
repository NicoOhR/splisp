#include <iostream>
#include <lexer.hpp>
#include <string>

int main() {
  std::string program = "(+ 3 3)";
  Lexer lex(program);
  return 0;
}
