cmake_minimum_required(VERSION 3.1)
project(test_package)

message("building test_package for conan2")

find_package(clangd_headers REQUIRED)

add_executable(${PROJECT_NAME} test_package.cpp)
target_link_libraries(${PROJECT_NAME} clangd_headers::clangd_headers)