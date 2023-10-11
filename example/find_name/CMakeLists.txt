add_executable(find_name EXCLUDE_FROM_ALL)
target_sources(find_name
  PRIVATE
  bootstrap.cpp
)
target_link_libraries(find_name
  PRIVATE
  cppcia_project_options
  cppcia_project_warnings
)

target_find_dependencies(find_name
  PRIVATE_CONFIG
  Microsoft.GSL
)
target_link_system_libraries(find_name
  PRIVATE
  llvm::llvm
  Microsoft.GSL::GSL
)