config: 
  cmake -S . -B build -DBUILD_TESTING=ON

build: 
  cmake --build build

test: 
  GTEST_COLOR=1 ctest --test-dir build --output-on-failure
