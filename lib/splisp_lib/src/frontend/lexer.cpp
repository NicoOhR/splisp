#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <frontend/lexer.hpp>
#include <iostream>
#include <optional>
#include <utility>

void printToken(Token tok) {
  std::cout << "Lexeme: " << tok.lexeme << " Kind: ";
  switch (tok.kind) {
  case (TokenKind::ident):
    std::cout << "Symbol";
    break;
  case (TokenKind::atoms):
    std::cout << " Atom";
    break;
  case (TokenKind::lparn):
  case (TokenKind::rparn):
    std::cout << "Parn";
    break;
  }
  std::cout << std::endl;
}

std::optional<Token> Lexer::next() {
  if (this->tokenized.empty()) {
    return std::nullopt;
  }
  Token tok = this->tokenized.front();
  this->tokenized.erase(this->tokenized.begin());
  return tok;
}

std::optional<Token> Lexer::peek() const {
  if (this->tokenized.empty()) {
    return std::nullopt;
  }
  Token tok = this->tokenized.front();
  return tok;
}

Lexer::Lexer(std::string program) {
  // preprocess string
  boost::replace_all(program, "(", " ( ");
  boost::replace_all(program, ")", " ) ");
  boost::algorithm::trim(program);

  std::vector<std::string> words;
  boost::split(words, program, boost::is_space(), boost::token_compress_on);
  Token curr;
  for (std::string str : words) {
    curr.lexeme = str;
    auto match = keywords.find(curr.lexeme);
    // first we match tokens for exact matches
    if (match != keywords.end()) {
      curr.kind = match->second;
    }
    // everything else has to be a non kword ident or non bool atom, for now
    // we don't consider variable names
    else {
      curr.kind = TokenKind::atoms;
    }
    printToken(curr);
    tokenized.push_back(curr);
  }
}
