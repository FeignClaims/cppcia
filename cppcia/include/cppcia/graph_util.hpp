#ifndef CPPCIA_GRAPH_UTIL_HPP
#define CPPCIA_GRAPH_UTIL_HPP

#include <concepts>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <graaflib/graph.h>
#include <graaflib/types.h>
#include <range/v3/all.hpp>

namespace cppcia {
template <typename Vertex, typename Edge, graaf::graph_type Graph_type>
auto merge_by(graaf::graph<Vertex, Edge, Graph_type>& self,
              graaf::graph<Vertex, Edge, Graph_type> const& other) -> graaf::graph<Vertex, Edge, Graph_type>& {
  std::unordered_map<Vertex, graaf::vertex_id_t> vertices_to_ids{
      self.get_vertices()
      | ranges::views::transform([](auto id_to_vertex) { return std::pair{id_to_vertex.second, id_to_vertex.first}; })
      | ranges::to<std::unordered_map>()};

  for (auto [_, vertex] : other.get_vertices()) {
    if (!vertices_to_ids.contains(vertex)) {
      auto id{self.add_vertex(vertex)};
      vertices_to_ids.try_emplace(vertex, id);
    }
  }

  for (auto [uv, edge] : other.get_edges()) {
    self.add_edge(vertices_to_ids[other.get_vertex(uv.first)], vertices_to_ids[other.get_vertex(uv.second)], edge);
  }

  return self;
}

template <typename Vertex, typename Edge, graaf::graph_type Graph_type>
[[nodiscard]] auto merge(graaf::graph<Vertex, Edge, Graph_type> const& lhs,
                         graaf::graph<Vertex, Edge, Graph_type> const& rhs) -> graaf::graph<Vertex, Edge, Graph_type> {
  graaf::graph<Vertex, Edge, Graph_type> result;
  merge_by(result, lhs);
  merge_by(result, rhs);
  return result;
}

template <typename Vertex, typename Edge, graaf::graph_type Graph_type>
auto merge_distinct_by(graaf::graph<Vertex, Edge, Graph_type>& self,
                       graaf::graph<Vertex, Edge, Graph_type> const& other) -> graaf::graph<Vertex, Edge, Graph_type>& {
  std::unordered_map<graaf::vertex_id_t, graaf::vertex_id_t> old_to_new_ids{};

  for (auto [old_id, vertex] : other.get_vertices()) {
    auto new_id{self.add_vertex(vertex)};
    old_to_new_ids.try_emplace(old_id, new_id);
  }

  for (auto [uv, edge] : other.get_edges()) {
    self.add_edge(old_to_new_ids[uv.first], old_to_new_ids[uv.second], edge);
  }

  return self;
}

template <typename Vertex, typename Edge, graaf::graph_type Graph_type>
[[nodiscard]] auto merge_distinct(graaf::graph<Vertex, Edge, Graph_type> const& lhs,
                                  graaf::graph<Vertex, Edge, Graph_type> const& rhs)
    -> graaf::graph<Vertex, Edge, Graph_type> {
  graaf::graph<Vertex, Edge, Graph_type> result;
  merge_distinct_by(result, lhs);
  merge_distinct_by(result, rhs);
  return result;
}

template <typename Vertex, typename Edge, graaf::graph_type Graph_type, std::invocable<Vertex> Mapper>
[[nodiscard]] auto map(graaf::graph<Vertex, Edge, Graph_type> const& graph,
                       Mapper&& mapper)  // NOLINT(*forward*)
    -> graaf::graph<std::invoke_result_t<Mapper, Vertex>, Edge, Graph_type> {
  using Mapped_vertex = std::invoke_result_t<Mapper, Vertex>;
  graaf::graph<Mapped_vertex, Edge, Graph_type> result;

  std::unordered_map<Mapped_vertex, graaf::vertex_id_t> mapped_vertices_to_ids{};
  for (auto [_, vertex] : graph.get_vertices()) {
    auto mapped_vertex{std::invoke(mapper, vertex)};
    auto iter{mapped_vertices_to_ids.find(mapped_vertex)};
    if (iter == mapped_vertices_to_ids.end()) {
      auto id{result.add_vertex(mapped_vertex)};
      mapped_vertices_to_ids.emplace_hint(iter, std::move(mapped_vertex), id);
    }
  }

  for (auto [uv, edge] : graph.get_edges()) {
    auto u_mapped_id{mapped_vertices_to_ids[std::invoke(mapper, graph.get_vertex(uv.first))]};
    auto v_mapped_id{mapped_vertices_to_ids[std::invoke(mapper, graph.get_vertex(uv.second))]};
    if (u_mapped_id != v_mapped_id) {
      result.add_edge(u_mapped_id, v_mapped_id, edge);
    }
  }

  return result;
}
}  // namespace cppcia

#endif