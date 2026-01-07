#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <iostream>
#include <string>

int main() {
  std::string program = "(define add (x y) (+ x y))";
  Lexer lex(program);
  Parser parser(std::move(lex));
  auto ast = parser.parse();
  std::cout << "--+--" << std::endl;
  ast::print_ast(ast);
  return 0;
}
