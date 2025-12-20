#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum TokenKind { ident, atoms, lparn, rparn };

struct Token {
  TokenKind kind;
  std::string raw;
  std::pair<uint32_t, uint32_t> span;
};

class Lexer {
public:
  Lexer(std::string program);
  Token next();
  Token peek() const;

private:
  std::vector<Token> tokenized;
};
