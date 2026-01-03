#pragma once
#include <optional>
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
  std::optional<Token> next();
  std::optional<Token> peek() const;

private:
  std::vector<Token> tokenized;
  const std::unordered_map<std::string, TokenKind> keywords = {
      {"(", TokenKind::lparn},
      {")", TokenKind::rparn},
      {"#t", TokenKind::atoms},
      {"#f", TokenKind::atoms}};
};
