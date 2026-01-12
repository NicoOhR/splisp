#include "frontend/ast.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#pragma

enum BindingKind { VALUE, FUNC };

struct Binding {
  BindingKind kind;
  uint64_t value;
};

struct SymbolTable {
  size_t scope_id;
  std::unordered_map<std::string, Binding> symbols;
  SymbolTable *parent;
};

class Scoper {
public:
  Scoper();
  void run(ast::AST &ast);
  void resolve(ast::AST &ast);
  void convert(ast::AST &ast);

private:
  SymbolTable table;
};
