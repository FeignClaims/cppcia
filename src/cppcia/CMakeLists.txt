add_library(cppcia_library INTERFACE)
target_include_interface_directories(cppcia_library include)
target_link_libraries(cppcia_library
  INTERFACE
  cppcia_project_options
  cppcia_project_warnings
)

target_link_system_libraries(cppcia_library
  INTERFACE
  llvm::llvm
)

add_executable(cppcia)
target_sources(cppcia
  PRIVATE
  src/bootstrap.cpp
)
target_link_libraries(cppcia
  PRIVATE
  cppcia_library
  cppcia_project_options
  cppcia_project_warnings
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()