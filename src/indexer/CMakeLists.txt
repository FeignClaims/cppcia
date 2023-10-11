add_library(indexer_library INTERFACE)
target_include_interface_directories(indexer_library include)
target_link_libraries(indexer_library
  INTERFACE
  cppcia_project_options
  cppcia_project_warnings
)

target_find_dependencies(indexer_library
  INTERFACE_CONFIG
  clangd_headers
)
target_link_libraries(indexer_library
  INTERFACE
  clangd_headers::clangd_headers
)

add_executable(indexer)
target_sources(indexer
  PRIVATE
  src/bootstrap.cpp
)
target_link_libraries(indexer
  PRIVATE
  indexer_library
  cppcia_project_options
  cppcia_project_warnings
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()