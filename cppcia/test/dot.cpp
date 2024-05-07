#include "cppcia/dot.hpp"

#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <clangd/Protocol.h>
#include <fmt/core.h>
#include <graaflib/graph.h>
#include <graaflib/types.h>

namespace cppcia {
TEST_CASE("format_to_in_dot", "[.dot]") {
  graaf::directed_graph<std::string, int> result;

  auto const vertex_1{result.add_vertex("a")};
  auto const vertex_2{result.add_vertex("b")};
  auto const vertex_3{result.add_vertex("c")};

  result.add_edge(vertex_1, vertex_2, 1);
  result.add_edge(vertex_1, vertex_3, 1);

  std::ostringstream oss;
  format_to_in_dot(
      oss,
      result,
      [](graaf::vertex_id_t id, std::string const& vertex) { return fmt::format("label=\"{}: {}\"", id, vertex); },
      [](graaf::edge_id_t id, int edge) { return fmt::format("label=\"{}-{}: {}\"", id.first, id.second, edge); });

  CHECK(oss.str() == R"dot(digraph {
  2 [label="2: c"];
  1 [label="1: b"];
  0 [label="0: a"];
  0 -> 2 [label="0-2: 1"];
  0 -> 1 [label="0-1: 1"];
}
)dot");
}
}  // namespace cppcia