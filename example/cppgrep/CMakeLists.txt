add_executable(cppgrep EXCLUDE_FROM_ALL)
target_sources(cppgrep
  PRIVATE
  bootstrap.cpp
)
target_link_libraries(cppgrep
  PRIVATE
  cppcia_project_options
  cppcia_project_warnings
)

target_find_dependencies(cppgrep
  PRIVATE_CONFIG
  llvm
  Microsoft.GSL
)
target_link_system_libraries(cppgrep
  PRIVATE
  llvm::llvm
  Microsoft.GSL::GSL
)