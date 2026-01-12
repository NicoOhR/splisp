#include <frontend/scoper.hpp>
#include <type_traits>
#include <variant>

Scoper::Scoper() {
  // create global scope
  table.scope_id = 0;
  table.parent = nullptr;
}

void Scoper::run(ast::AST &ast) {
  // recurse from the top of the AST; lambda list forms define their own
  // lexical scope. Using C++23 features implemented only by the legally insane
  size_t curr_idx = 0;
  auto visit = [&](this auto &&self, ast::SExp &sexp,
                   SymbolTable *parent) -> void {
    std::visit(
        [&](auto &node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, ast::List>) {
            // for any list that we encounter, recur down to that level
            bool is_lambda = false;
            if (!node.list.empty()) {
              if (auto *sym =
                      std::get_if<ast::Symbol>(&node.list.front()->node)) {
                if (auto *kw = std::get_if<ast::Keyword>(&sym->value)) {
                  is_lambda = (*kw == ast::Keyword::lambda);
                }
              }
            }

            if (is_lambda) {
              curr_idx++;
              SymbolTable this_scope = {.scope_id = curr_idx, .parent = parent};
              for (auto &child : node.list) {
                self(*child, &this_scope);
              }
              return;
            }

            for (auto &child : node.list) {
              self(*child, parent);
            }
            return;
          }
          if constexpr (std::is_same_v<T, ast::Symbol>) {
            // symbols don't have their own lexical scope so we can skip it
            return;
          }
        },
        sexp.node);
  };
  for (auto &root : ast) {
    visit(*root, &this->table);
  }
}
