#include "frontend/ast.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#pragma

struct symbol_table {
  size_t scope_id;
  std::unordered_map<std::string, uint64_t> symbols;
  symbol_table &parent;
};

class Scoper {
public:
  void run(ast::AST &ast);
  void resolve(ast::AST &ast);

private:
  symbol_table table;
};
