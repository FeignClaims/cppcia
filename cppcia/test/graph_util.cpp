#include "cppcia/graph_util.hpp"

#include <functional>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <graaflib/graph.h>

namespace cppcia {
TEST_CASE("merge_by", "[graph_util]") {
  auto self{std::invoke([]() {
    graaf::directed_graph<std::string, int> initer{};
    auto vertex_1{initer.add_vertex("a")};
    auto vertex_2{initer.add_vertex("b")};
    auto vertex_3{initer.add_vertex("c")};

    initer.add_edge(vertex_1, vertex_2, 1);
    initer.add_edge(vertex_1, vertex_3, 2);

    return initer;
  })};

  merge_by(self, std::invoke([]() {
             graaf::directed_graph<std::string, int> initer{};
             auto vertex_1{initer.add_vertex("a")};
             auto vertex_2{initer.add_vertex("b")};
             auto vertex_3{initer.add_vertex("d")};

             initer.add_edge(vertex_1, vertex_2, 3);
             initer.add_edge(vertex_1, vertex_3, 4);

             return initer;
           }));

  CHECK(self.vertex_count() == 4);
  CHECK(self.edge_count() == 3);
}

TEST_CASE("merge", "[graph_util]") {
  auto graph{merge(std::invoke([]() {
                     graaf::directed_graph<std::string, int> initer{};
                     auto vertex_1{initer.add_vertex("a")};
                     auto vertex_2{initer.add_vertex("b")};
                     auto vertex_3{initer.add_vertex("c")};

                     initer.add_edge(vertex_1, vertex_2, 1);
                     initer.add_edge(vertex_1, vertex_3, 2);

                     return initer;
                   }),
                   std::invoke([]() {
                     graaf::directed_graph<std::string, int> initer{};
                     auto vertex_1{initer.add_vertex("a")};
                     auto vertex_2{initer.add_vertex("b")};
                     auto vertex_3{initer.add_vertex("d")};

                     initer.add_edge(vertex_1, vertex_2, 3);
                     initer.add_edge(vertex_1, vertex_3, 4);

                     return initer;
                   }))};

  CHECK(graph.vertex_count() == 4);
  CHECK(graph.edge_count() == 3);
}

TEST_CASE("merge_distinct_by", "[graph_util]") {
  auto self{std::invoke([]() {
    graaf::directed_graph<std::string, int> initer{};
    auto vertex_1{initer.add_vertex("a")};
    auto vertex_2{initer.add_vertex("b")};
    auto vertex_3{initer.add_vertex("c")};

    initer.add_edge(vertex_1, vertex_2, 1);
    initer.add_edge(vertex_1, vertex_3, 2);

    return initer;
  })};

  merge_distinct_by(self, std::invoke([]() {
                      graaf::directed_graph<std::string, int> initer{};
                      auto vertex_1{initer.add_vertex("a")};
                      auto vertex_2{initer.add_vertex("b")};
                      auto vertex_3{initer.add_vertex("d")};

                      initer.add_edge(vertex_1, vertex_2, 3);
                      initer.add_edge(vertex_1, vertex_3, 4);

                      return initer;
                    }));

  CHECK(self.vertex_count() == 6);
  CHECK(self.edge_count() == 4);
}

TEST_CASE("merge_distinct", "[graph_util]") {
  auto graph{merge_distinct(std::invoke([]() {
                              graaf::directed_graph<std::string, int> initer{};
                              auto vertex_1{initer.add_vertex("a")};
                              auto vertex_2{initer.add_vertex("b")};
                              auto vertex_3{initer.add_vertex("c")};

                              initer.add_edge(vertex_1, vertex_2, 1);
                              initer.add_edge(vertex_1, vertex_3, 2);

                              return initer;
                            }),
                            std::invoke([]() {
                              graaf::directed_graph<std::string, int> initer{};
                              auto vertex_1{initer.add_vertex("a")};
                              auto vertex_2{initer.add_vertex("b")};
                              auto vertex_3{initer.add_vertex("d")};

                              initer.add_edge(vertex_1, vertex_2, 3);
                              initer.add_edge(vertex_1, vertex_3, 4);

                              return initer;
                            }))};

  CHECK(graph.vertex_count() == 6);
  CHECK(graph.edge_count() == 4);
}

TEST_CASE("map", "[graph_util]") {
  auto graph{map(std::invoke([]() {
                   graaf::directed_graph<int, int> initer{};
                   // NOLINTBEGIN(*magic-number*)
                   auto vertex_1{initer.add_vertex(1)};
                   auto vertex_2{initer.add_vertex(2)};
                   auto vertex_3{initer.add_vertex(3)};
                   auto vertex_4{initer.add_vertex(4)};
                   auto vertex_5{initer.add_vertex(5)};
                   auto vertex_6{initer.add_vertex(6)};
                   // NOLINTEND(*magic-number*)

                   initer.add_edge(vertex_1, vertex_3, 1);
                   initer.add_edge(vertex_1, vertex_5, 3);
                   initer.add_edge(vertex_3, vertex_5, 2);
                   initer.add_edge(vertex_2, vertex_4, 3);
                   initer.add_edge(vertex_2, vertex_6, 3);
                   initer.add_edge(vertex_4, vertex_6, 3);

                   return initer;
                 }),
                 [](int value) { return value % 2 == 0; })};

  CHECK(graph.vertex_count() == 2);
  CHECK(graph.edge_count() == 0);
}
}  // namespace cppcia