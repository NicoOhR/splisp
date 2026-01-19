#include "frontend/ast.hpp"
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#pragma once

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
  // find the current scoped symbol table
  SymbolTable *find_table(size_t id);
  // search for the ident moving up the symbol table tree
  Binding search(std::string ident, size_t lowest_scope);
  SymbolTable root;
  size_t next_binding_id = 0;
};

void print_symbol_table(std::ostream &os, const SymbolTable &table,
                        size_t indent = 0);
