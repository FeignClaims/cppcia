add_executable(ast_dump EXCLUDE_FROM_ALL)
target_sources(ast_dump
  PRIVATE
  bootstrap.cpp
)
target_link_libraries(ast_dump
  PRIVATE
  cppcia_project_options
  cppcia_project_warnings
)

target_link_system_libraries(ast_dump
  PRIVATE
  llvm::llvm
)