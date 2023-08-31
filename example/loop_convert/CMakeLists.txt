add_executable(loop_convert EXCLUDE_FROM_ALL)
target_sources(loop_convert
  PRIVATE
  bootstrap.cpp
)
target_link_libraries(loop_convert
  PRIVATE
  cppcia_project_options
  cppcia_project_warnings
)

target_find_dependencies(loop_convert
  PRIVATE_CONFIG
  llvm
  Microsoft.GSL
)
target_link_system_libraries(loop_convert
  PRIVATE
  llvm::llvm
  Microsoft.GSL::GSL
)