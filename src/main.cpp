#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <iostream>
#include <string>

int main() {
  std::string program = "(if (< 2 3) (+ 3 4) 5)";
  Lexer lex(program);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  std::cout << "--+--" << std::endl;
  ast::print_ast(ast);
  return 0;
}
