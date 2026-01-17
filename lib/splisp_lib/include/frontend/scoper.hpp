#include "frontend/ast.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#pragma

enum BindingKind { VALUE, FUNC };
// given an AST
// replace all bindings with unique SymbolID's

struct Binding {
  BindingKind kind;
  uint64_t value;
};

struct SymbolTable {
  size_t scope_id;
  std::unordered_map<std::string, Binding> symbols;
  std::vector<std::unique_ptr<SymbolTable>> children;
  SymbolTable *parent;
};

class Scoper {
public:
  Scoper();
  void run(ast::AST &ast);
  void resolve(ast::AST &ast);

private:
  SymbolTable root;
  size_t next_binding_id = 0;
};
