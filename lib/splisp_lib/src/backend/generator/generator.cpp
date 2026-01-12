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
    emit_sexp(*sexp);
  }
}

void Generator::emit_sexp(const ast::SExp &sexp) {
  std::visit(
      [this](const auto &p) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, ast::List>) {
          emit_list(p);
        } else if constexpr (std::is_same_v<T, ast::Symbol>) {
          emit_symbol(p);
        }
      },
      sexp.node);
}
void Generator::emit_symbol(const ast::Symbol &sym) {
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

void Generator::emit_keyword(const ast::List &list) {
  if (const auto *head_symbol =
          std::get_if<ast::Symbol>(&list.list.front()->node)) {
    if (const auto *head_kword =
            std::get_if<ast::Keyword>(&head_symbol->value)) {
      switch (*head_kword) {
      case (ast::Keyword::if_expr):
        // List<Sexp(Keyword(if)), Sexp(cond), Sexp(then), Sexp(else)>
        // 0. emit PUSH X (will calculate X)
        // 1. emit condition to program vector
        // 2. emit CJMP (note length here)
        // 3. emit (else)
        // 4. emit JMP Y (will calculate Y)
        // 5. emit (then)
        // X = length before (5), Y = length after (5)
        this->program.push_back(ISA::Instruction{.op = ISA::Operation::PUSH});
        size_t cjmp_idx =
            this->program.size() - 1; // note the index of jump address
        const auto &cond = *list.list[1];
        emit_sexp(cond);
        this->program.push_back(ISA::Instruction{.op = ISA::Operation::CJMP,
                                                 .operand = std::nullopt});
        const auto &else_code = *list.list[3];
        emit_sexp(else_code);
        this->program.push_back(ISA::Instruction{.op = ISA::Operation::PUSH});
        size_t jmp_idx = this->program.size() - 1;
        this->program.push_back(ISA::Instruction{.op = ISA::Operation::JMP,
                                                 .operand = std::nullopt});
        const auto &then_code = *list.list[2];
        size_t X = (jmp_idx + 2) * 9;
        emit_sexp(then_code);
        size_t Y = (this->program.size()) * 9;
        this->program[cjmp_idx].operand = X;
        this->program[jmp_idx].operand = Y;
      }
    }
  }
}

void Generator::emit_function(const ast::List &list) {}

void Generator::emit_list(const ast::List &list) {
  /*
   * if the list is nonempty, and the first is a keyword, then we interpret
   * the rest of the list as a keyword dispatch
   *
   * if the first is an operation, than we treat the rest of the list as a
   * function application dispatch
   *
   * otherwise it's just a list
   */
  if (!list.list.empty()) {
    if (const auto *head_symbol =
            std::get_if<ast::Symbol>(&list.list.front()->node)) {

      if (const auto *head_kword =
              std::get_if<ast::Keyword>(&head_symbol->value)) {
        emit_keyword(list);
      } else if (const auto *head_op =
                     std::get_if<std::string>(&head_symbol->value)) {
        emit_function(list);
      }
    }
  }
  for (auto &list_sexp : list.list) {
    emit_sexp(*list_sexp);
  }
}
