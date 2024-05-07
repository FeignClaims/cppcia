#include "cppcia/dot.hpp"
#include "cppcia/extractor.hpp"
#include "cppcia/reference.hpp"
#include "cppcia/referencer.hpp"

#include <fstream>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <llvm/Support/raw_ostream.h>
#include <support/Path.h>

namespace cppcia {
TEST_CASE("extractor", "[.real]") {
  Extractor extractor{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                     "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  extractor.update_file(file, read_file(file));

  auto items{extractor.find_callers(extractor.prepare_call_hierarchy(file, {69, 7}))};  // NOLINT(*magic-number*)
  for (auto& item : items) {
    llvm::outs() << item.from.name << '\n';
    llvm::outs() << "  " << item.from.range << '\n';
    llvm::outs() << "  " << item.from.uri.file() << '\n';
  }
}

TEST_CASE("referencer", "[.real]") {
  Referencer referencer{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                       "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  referencer.update_file(file, read_file(file));

  auto reference{referencer.query_location(file, {101, 21})};  // NOLINT(*magic-number*)
  REQUIRE(reference.has_value());

  Reference_tree references{referencer.find_caller_hierarchies(*reference)};

  std::ofstream ofile{"/Users/feignclaims/code/cppcia/graph.dot"};
  format_to_in_dot(ofile,
                   to_graph(references, Edge_type::solid, /*reverse_edge=*/false),
                   Reference_writer{"/Users/feignclaims/code/cpp/lefticus_cmake_template"});
}

TEST_CASE("find_type", "[.real]") {
  Referencer referencer{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                       "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  referencer.update_file(file, read_file(file));

  auto reference{referencer.query_location(file, {96, 18})};  // NOLINT(*magic-number*)
  REQUIRE(reference.has_value());

  Reference_tree references{referencer.find_type(*reference), {}};

  std::ofstream ofile{"/Users/feignclaims/code/cppcia/graph.dot"};
  format_to_in_dot(ofile,
                   to_graph(references, Edge_type::solid, /*reverse_edge=*/false),
                   Reference_writer{"/Users/feignclaims/code/cpp/lefticus_cmake_template"});
}

TEST_CASE("query_file", "[.real]") {
  Referencer referencer{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                       "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  referencer.update_file(file, read_file(file));

  Reference_tree references{referencer.query_file(file)};

  std::ofstream ofile{"/Users/feignclaims/code/cppcia/graph.dot"};
  format_to_in_dot(ofile,
                   to_graph(references, Edge_type::solid, /*reverse_edge=*/false),
                   Reference_writer{"/Users/feignclaims/code/cpp/lefticus_cmake_template"});
}

TEST_CASE("find_container", "[.real]") {
  Referencer referencer{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                       "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  referencer.update_file(file, read_file(file));

  auto reference{referencer.query_location(file, {95, 18})};  // NOLINT(*magic-number*)
  REQUIRE(reference.has_value());

  Reference_tree references{referencer.find_container(*reference), {}};

  std::ofstream ofile{"/Users/feignclaims/code/cppcia/graph.dot"};
  format_to_in_dot(ofile,
                   to_graph(references, Edge_type::solid, /*reverse_edge=*/false),
                   Reference_writer{"/Users/feignclaims/code/cpp/lefticus_cmake_template"});
}

TEST_CASE("find_container_tree", "[.real]") {
  Referencer referencer{make_extractor("/Users/feignclaims/code/cpp/lefticus_cmake_template/test.idx",
                                       "/Users/feignclaims/code/cpp/lefticus_cmake_template/build/")};

  clang::clangd::PathRef file{"/Users/feignclaims/code/cpp/lefticus_cmake_template/src/ftxui_sample/main.cpp"};
  referencer.update_file(file, read_file(file));

  auto reference{referencer.query_location(file, {95, 18})};  // NOLINT(*magic-number*)
  REQUIRE(reference.has_value());

  Reference_tree references{referencer.find_container(*reference), {}};

  std::ofstream ofile{"/Users/feignclaims/code/cppcia/graph.dot"};
  format_to_in_dot(ofile,
                   to_graph(references, Edge_type::solid, /*reverse_edge=*/false),
                   Reference_writer{"/Users/feignclaims/code/cpp/lefticus_cmake_template"});
}
}  // namespace cppcia