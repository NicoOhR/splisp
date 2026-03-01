enable_testing()

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SPLISP_TEST_SOURCES
  lexer_parser_tests.cpp
  lowerer_tests.cpp
  scoper_tests.cpp
)

if(SPLISP_BUILD_VM)
  list(APPEND SPLISP_TEST_SOURCES
    vm_program_tests.cpp
    vm_stack_tests.cpp
  )
endif()

add_executable(splisp_tests ${SPLISP_TEST_SOURCES})

target_link_libraries(splisp_tests PRIVATE splisp_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(splisp_tests)
