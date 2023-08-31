cmake_minimum_required(VERSION 3.25)
project(cppcia VERSION 0.0.1 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Customize project_options
include(CustomizedProjectOptions)

# Options
option(BUILD_TESTING "Building the testing tree." OFF)
option(BUILD_FUZZ_TESTING "Enable the fuzz tests" OFF)

# workaround: RPATH stuff
# !!!NOT RECOMMENDED!!! see https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C%2B%2B.md
include(Rpath)

# Add LLVM dependencies
include(LLVM)

# Include CTest before any tests
include(CTest)

# Add handy test wrappers
include(AddTest)

# Common test config
add_test_config(common
  DEPENDENCIES_CONFIG
  ut

  LIBRARIES
  cppcia_project_options
  cppcia_project_warnings
  boost-ext-ut::ut
)

# Add src
add_subdirectory(cppcia)
add_subdirectory(example)

# Set the startup project for the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT cppcia)

# Package the project
package_project(
  TARGETS
  cppcia
)

set(CPACK_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)