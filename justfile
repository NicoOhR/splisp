config: 
  cmake -S . -B build -DBUILD_TESTING=ON

build: 
  cmake --build build

n_vm: 
  cmake -S  . -B build -DSPLISP_BUILD_VM=OFF

test: 
  GTEST_COLOR=1 ctest --test-dir build --output-on-failure
