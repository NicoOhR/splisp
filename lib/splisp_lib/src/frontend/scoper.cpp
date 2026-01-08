#include <frontend/scoper.hpp>

Scoper::Scoper() {
  // create global scope
  table.scope_id = 0;
  table.parent = nullptr;
}

void Scoper::run(ast::AST &ast) {
  // iterate from the top of the AST, let, define, and lambda define their own
  // lexical scope
  for (auto &root : ast) {
  }
}
