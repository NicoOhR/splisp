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
  std::vector<std::string> strings;
  boost::split(strings, program, boost::is_any_of(" "));
  for (std::string str : strings) {
    std::cout << str << " ";
  }
}
