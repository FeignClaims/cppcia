# Seperate library part and `main()` function to simplify testing.
# If not doing so, you can't test because there's already a `main()` function.

# Library part
add_library(cppcia_library INTERFACE)
target_include_interface_directories(cppcia_library) # Includes
target_link_libraries(cppcia_library
  INTERFACE
  cppcia_project_options
  cppcia_project_warnings
)

# Executable part
add_executable(cppcia)
target_sources(cppcia
  PRIVATE
  src/bootstrap.cpp # Just for main() function
)
target_link_libraries(cppcia
  PRIVATE
  cppcia_library
  cppcia_project_options
  cppcia_project_warnings
)

target_find_dependencies(cppcia # Find dependencies
  PRIVATE_CONFIG
  llvm
)
target_link_system_libraries(cppcia # Link dependencies
  PRIVATE
  llvm::llvm
)

# Tests
if(BUILD_TESTING)
  add_subdirectory(test)
endif()