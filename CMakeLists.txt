cmake_minimum_required(VERSION 3.25)
project(cppcia VERSION 1.0.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Customize project_options
include(enable_customized_project_options)

# Options
option(BUILD_TESTING "Building the testing tree." ON)

# workaround: RPATH stuff
# !!!NOT RECOMMENDED!!! see https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C%2B%2B.md
include(rpath)

# Add LLVM dependencies
include(llvm)

# Include CTest before any tests
include(CTest)

# Add handy test wrappers
include(add_test)

# Add src
add_subdirectory(cppcia)

# Package the project
package_project(
  TARGETS
  cppcia
)

detect_architecture(arch)
set(CPACK_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${arch}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)