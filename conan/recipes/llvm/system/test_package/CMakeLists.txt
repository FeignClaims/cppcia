cmake_minimum_required(VERSION 3.21)
project(test_package)

message("building test_package for conan2")

find_package(llvm_system)

add_executable(test_package)
target_sources(test_package
  PRIVATE
  test_package.cpp
)
target_link_libraries(test_package
  PRIVATE
  llvm::llvm
)
target_compile_features(test_package
  PRIVATE
  cxx_std_20
)