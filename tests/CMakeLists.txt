enable_testing()

add_executable(splisp_tests
  lexer_parser_tests.cpp
)

target_link_libraries(splisp_tests PRIVATE splisp_lib)

add_test(NAME splisp_tests COMMAND splisp_tests)
