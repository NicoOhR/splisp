#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

enum TokenKind { ident, atoms, lparn, rparn };

struct Token {
  TokenKind kind;
  std::string lexeme;
};

void printToken(Token tok);

class Lexer {
public:
  Lexer(std::string program);
  Token next();
  Token peek() const;

private:
  std::vector<Token> tokenized;
  const std::unordered_map<std::string, TokenKind> keywords = {
      {"(", TokenKind::lparn},  {")", TokenKind::rparn},
      {"+", TokenKind::ident},  {"-", TokenKind::ident},
      {"*", TokenKind::ident},  {"/", TokenKind::ident},
      {"#t", TokenKind::atoms}, {"#f", TokenKind::atoms}};
};
