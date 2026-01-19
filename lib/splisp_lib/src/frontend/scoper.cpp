#include "frontend/ast.hpp"
#include <algorithm>
#include <frontend/scoper.hpp>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

Scoper::Scoper() {
  // create global scope
  root.scope_id = 0;
  root.parent = nullptr;
}

void Scoper::run(ast::AST &ast) {
  // recurse from the top of the AST; lambda list forms define their own
  // lexical scope. Using C++23 features implemented only by the legally insane
  size_t curr_idx = 0;
  auto visit = [&, curr_idx, scoper = this](this auto &&self, ast::SExp &sexp,
                                            SymbolTable *parent) -> void {
    std::visit(
        [&](auto &node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::List>) {
            // for any list that we encounter, recur down to that level
            if (!node.list.empty()) {
              if (auto *sym =
                      std::get_if<ast::Symbol>(&node.list.front()->node)) {
                if (auto *kw = std::get_if<ast::Keyword>(&sym->value)) {
                  switch (*kw) {
                  case (ast::Keyword::lambda): {
                    // List(Kword(Lambda) List(Symbols(Strings(Args))) ...)
                    if (auto *args =
                            std::get_if<ast::List>(&node.list.at(1)->node)) {
                      std::unordered_map<std::string, Binding> syms;
                      for (size_t i = 0; i < args->list.size(); i++) {
                        if (auto *arg = std::get_if<ast::Symbol>(
                                &args->list.at(i)->node)) {
                          if (auto *ident =
                                  std::get_if<std::string>(&arg->value)) {
                            syms[*ident] =
                                Binding{.kind = BindingKind::VALUE,
                                        .value = scoper->next_binding_id++};
                          }
                        }
                      }
                      auto child_scope = std::make_unique<SymbolTable>();
                      child_scope->scope_id = ++curr_idx;
                      child_scope->symbols = std::move(syms);
                      child_scope->parent = parent;
                      sexp.scope_id = curr_idx;
                      SymbolTable *child_ptr = child_scope.get();
                      parent->children.push_back(std::move(child_scope));
                      for (auto &child : node.list) {
                        self(*child, child_ptr);
                      }
                      return;
                    }
                    break;
                  }
                  case (ast::Keyword::define): {
                    // List(Kword(Define) Symbol(name) ...)
                    if (auto *name =
                            std::get_if<ast::Symbol>(&node.list.at(1)->node)) {
                      if (auto *ident =
                              std::get_if<std::string>(&name->value)) {
                        parent->symbols[*ident] =
                            Binding{.kind = BindingKind::FUNC,
                                    .value = scoper->next_binding_id++};
                      }
                    }
                    if (auto *args =
                            std::get_if<ast::List>(&node.list.at(2)->node)) {
                      std::unordered_map<std::string, Binding> syms;
                      for (size_t i = 0; i < args->list.size(); i++) {
                        if (auto *arg = std::get_if<ast::Symbol>(
                                &args->list.at(i)->node)) {
                          if (auto *ident =
                                  std::get_if<std::string>(&arg->value)) {
                            syms[*ident] =
                                Binding{.kind = BindingKind::VALUE,
                                        .value = scoper->next_binding_id++};
                          }
                        }
                      }
                      auto child_scope = std::make_unique<SymbolTable>();
                      child_scope->scope_id = ++curr_idx;
                      child_scope->symbols = std::move(syms);
                      child_scope->parent = parent;
                      sexp.scope_id = curr_idx;
                      SymbolTable *child_ptr = child_scope.get();
                      parent->children.push_back(std::move(child_scope));
                      for (auto &child : node.list) {
                        self(*child, child_ptr);
                      }
                      return;
                    }
                    break;
                  }
                  default:
                    break;
                  }
                }
              }
            }
            for (auto &child : node.list) {
              self(*child, parent);
            }
          }
          if constexpr (std::is_same_v<T, ast::Symbol>) {
            return;
          }
        },
        sexp.node);
  };
  for (auto &root : ast) {
    visit(*root, &this->root);
  }
}

void Scoper::resolve(ast::AST &ast) {
  auto visit = [&, scoper = this](this auto &&self, ast::SExp &sexp,
                                  size_t curr_scope) -> void {
    std::visit(
        [&](auto &node) -> void {
          using T = std::decay_t<decltype(node)>;
          // 1. go down to a symbol and identify which scope id it's part of
          // a) maintain a current_scope as a parameter, at every SExp update it
          // if that field is populated
          // 2. find in table, try to resolve from the symbol
          // 3. if its not found, go up to parent and repeat
          if constexpr (std::is_same_v<T, ast::List>) {
            if (auto *sym =
                    std::get_if<ast::Symbol>(&node.list.front()->node)) {
              if (auto *kw = std::get_if<ast::Keyword>(&sym->value)) {
                if (*kw == ast::Keyword::lambda ||
                    *kw == ast::Keyword::define) {
                  if (sexp.scope_id) {
                    for (size_t i = 1; i < node.list.size(); i++) {
                      self(*node.list.at(i), *sexp.scope_id);
                    }
                  }
                }
              }
            }
          }
          if constexpr (std::is_same_v<T, ast::Symbol>) {
            if (auto *ident = std::get_if<std::string>(&node.value)) {
              auto binding = scoper->search(*ident, curr_scope);
              node.value = ast::SymbolID{binding.value};
            }
          }
        },
        sexp.node);
  };
  for (auto &root : ast) {
    visit(*root, 0);
  }
}
SymbolTable *Scoper::find_table(size_t id) {
  // i'm relatively sure that the scope id's are linear so a full DFS is
  // probably not needed, all the same.
  std::stack<SymbolTable *> stack;
  std::vector<size_t> visited;
  stack.push(&root);
  while (!stack.empty()) {
    auto curr = stack.top();
    stack.pop();
    if (curr->scope_id == id) {
      return curr;
    }
    if (std::find(visited.begin(), visited.end(), curr->scope_id) ==
        visited.end()) {
      visited.push_back(curr->scope_id);
      for (auto &child : curr->children) {
        stack.push(child.get());
      }
    }
  }
  throw std::invalid_argument("scope not found");
};

Binding Scoper::search(std::string ident, size_t lowest_scope) {
  auto table = Scoper::find_table(lowest_scope);
  while (table->parent != nullptr) {
    if (auto it = table->symbols.find(ident); it != table->symbols.end()) {
      return it->second;
    } else {
      table = table->parent;
    }
  }
  throw std::invalid_argument("symbol not found in any scope");
};

void print_symbol_table(std::ostream &os, const SymbolTable &table,
                        size_t indent) {
  const auto kind_name = [](BindingKind kind) -> const char * {
    switch (kind) {
    case BindingKind::VALUE:
      return "value";
    case BindingKind::FUNC:
      return "func";
    default:
      return "unknown";
    }
  };

  const std::string pad(indent, ' ');
  os << pad << "scope " << table.scope_id << '\n';
  for (const auto &entry : table.symbols) {
    os << pad << "  " << entry.first << ": " << kind_name(entry.second.kind)
       << " id=" << entry.second.value << '\n';
  }
  for (const auto &child : table.children) {
    print_symbol_table(os, *child, indent + 2);
  }
}
