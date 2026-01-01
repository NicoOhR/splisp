add_library(splisp_lib STATIC
  src/frontend/ast.cpp
  src/frontend/lexer.cpp
  src/frontend/parser.cpp
  src/backend/vm/stack.cpp
  src/backend/isa/isa.cpp
)

find_package(Boost 1.74 REQUIRED)
target_link_libraries(splisp_lib PRIVATE Boost::boost)

target_include_directories(splisp_lib 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(splisp_lib PUBLIC cxx_std_20)

target_compile_options(splisp_lib PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
  -g
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)
