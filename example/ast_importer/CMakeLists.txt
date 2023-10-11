add_executable(ast_importer EXCLUDE_FROM_ALL)
target_sources(ast_importer
  PRIVATE
  bootstrap.cpp
)
target_link_libraries(ast_importer
  PRIVATE
  cppcia_project_options
  cppcia_project_warnings
)

target_link_system_libraries(ast_importer
  PRIVATE
  llvm::llvm
)