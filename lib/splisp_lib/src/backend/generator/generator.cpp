#include <backend/generator/generator.hpp>
#include <backend/isa/isa.hpp>
#include <cstdint>
#include <frontend/ast.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

void Generator::generate() {
  for (auto &sexp : this->ast) {
    lower_sexp(*sexp);
  }
}

void Generator::lower_sexp(const ast::SExp &sexp) {
  std::visit(
      [this](const auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ast::List>) {
          lower_list(p);
        } else if constexpr (std::is_same_v<T, ast::Symbol>) {
          lower_symbol(p);
        }
      },
      sexp.node);
}
void Generator::lower_symbol(const ast::Symbol &sym) {
  ISA::Instruction instr;
  std::visit(
      [this, &instr](const auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, uint64_t>) {
          instr.op = ISA::Operation::PUSH;
          instr.operand = p;
          program.push_back(instr);
        } else if constexpr (std::is_same_v<T, ast::Keyword>) {
          throw std::invalid_argument("keyword not where expected");
        } else if constexpr (std::is_same_v<T, bool>) {
          instr.op = ISA::Operation::PUSH;
          instr.operand = p ? 1 : 0;
          program.push_back(instr);
        } else if constexpr (std::is_same_v<T, std::string>) {
          throw std::invalid_argument(
              "function application not where expected");
        }
      },
      sym.value);
}

void Generator::lower_keyword(const ast::Keyword kword) {}

void Generator::lower_function(const std::string func) {}

void Generator::lower_list(const ast::List &list) {
  /*
   * if the list is nonempty, and the first is a keyword, then we interpret the
   * rest of the list as a keyword dispatch
   *
   * if the first is an operation, than we treat the rest of the least as a
   * function application dispatch
   *
   * otherwise it's just a list
   */
  if (!list.list.empty()) {
    if (const auto *head_symbol =
            std::get_if<ast::Symbol>(&list.list.front()->node)) {
      const auto *head_kword = std::get_if<ast::Keyword>(&head_symbol->value);
      if (head_kword != nullptr) {
        lower_keyword(*head_kword);
        return;
      }
    } else if (const auto *head_op =
                   std::get_if<std::string>(&head_symbol->value)) {
      lower_function(*head_op);
    }
  }
  for (auto &sexp : list.list) {
    lower_sexp(*sexp);
  }
}
