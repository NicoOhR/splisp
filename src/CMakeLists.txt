add_executable(splisp_lang 
  main.cpp
)

target_compile_features(splisp_lang PRIVATE cxx_std_20)

target_link_libraries(splisp_lang PRIVATE splisp_lib)

target_compile_options(splisp_lang PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)
