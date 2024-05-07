add_test_config(cppcia
  INCLUDES
  test_util/include

  SOURCES
  test_util/src/annotations.cpp
  test_util/src/extractor.cpp
  test_util/src/referencer.cpp

  DEPENDENCIES_CONFIG
  Catch2

  LIBRARIES
  cppcia_project_options
  cppcia_project_warnings
  cppcia_library
  Catch2::Catch2WithMain
)

function(test_cppcia_library source_name)
  add_library_test(cppcia_library ${source_name} CONFIGS cppcia SOURCES "${source_name}.cpp")
endfunction()

test_cppcia_library(extractor)
test_cppcia_library(graph_util)
test_cppcia_library(referencer)

test_cppcia_library(dot)
set_tests_properties(test.cppcia_library.dot PROPERTIES PASS_REGULAR_EXPRESSION "No tests ran")
test_cppcia_library(real)
set_tests_properties(test.cppcia_library.real PROPERTIES PASS_REGULAR_EXPRESSION "No tests ran")

add_executable_test(cppcia no_arg WILL_FAIL)