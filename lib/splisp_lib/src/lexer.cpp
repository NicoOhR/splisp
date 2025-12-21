#include "lexer.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <iostream>
#include <utility>

Lexer::Lexer(std::string program) {
  boost::replace_all(program, "(", " ( ");
  boost::replace_all(program, ")", " ) ");
  std::vector<std::string> words;
  boost::split(words, program, boost::is_any_of(" "));
  Token curr;
  for (std::string str : words) {
    curr.raw = str;
    auto match = keywords.find(curr.raw);
    // first we match tokens for exact matches
    if (match != keywords.end()) {
      curr.kind = match->second;
    }
    // everything else has to be a non kword ident or non bool atom, for now we
    // don't consider variable names
    else {
      curr.kind = TokenKind::atoms;
    }
    tokenized.push_back(curr);
  }
}
