#ifndef CPPCIA_DOT_HPP
#define CPPCIA_DOT_HPP

#include "cppcia/detail/raw_streamed.hpp"
#include "cppcia/reference.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <gsl/gsl>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <graaflib/edge.h>
#include <graaflib/graph.h>
#include <graaflib/types.h>
#include <llvm/ADT/StringRef.h>
#include <magic_enum/magic_enum.hpp>

namespace cppcia {
namespace detail {
  [[nodiscard]] constexpr auto graph_type_to_string(graaf::graph_type const& type) -> char const* {
    using enum graaf::graph_type;
    switch (type) {
      case DIRECTED:
        return "digraph";
      case UNDIRECTED:
        return "graph";
    }
    return "";  // FIXME: unreachable
  }

  [[nodiscard]] constexpr auto graph_type_to_edge_specifier(graaf::graph_type const& type) -> char const* {
    using enum graaf::graph_type;
    switch (type) {
      case DIRECTED:
        return "->";
      case UNDIRECTED:
        return "--";
    }
    return "";  // FIXME: unreachable
  }
}  // namespace detail

inline namespace cpo {
  inline constexpr auto empty_writer{[](auto /*id*/, auto&& /*value*/) {
    return "";
  }};

  inline constexpr auto edge_type_writer{[](auto /*id*/, Edge_type edge_type) -> std::string {
    return fmt::format(R"dot(style="{}")dot", magic_enum::enum_name(edge_type));
  }};
}  // namespace cpo

template <typename Vertex,
          typename Edge,
          graaf::graph_type Graph_type,
          typename Vertex_writer_t,
          typename Edge_writer_t = decltype(empty_writer)>
  requires std::is_invocable_r_v<std::string, Vertex_writer_t const&, graaf::vertex_id_t, Vertex const&>
           && std::is_invocable_r_v<std::string,
                                    Edge_writer_t const&,
                                    graaf::edge_id_t const&,
                                    typename graaf::graph<Vertex, Edge, Graph_type>::edge_t const&>
void format_to_in_dot(std::ostream& ostream,
                      graaf::graph<Vertex, Edge, Graph_type> const& graph,
                      Vertex_writer_t vertex_writer,
                      Edge_writer_t edge_writer = empty_writer) {
  std::ostreambuf_iterator<char> iter{ostream};

  iter = fmt::format_to(iter, "{} {{\n", detail::graph_type_to_string(Graph_type));

  for (auto const& [vertex_id, vertex] : graph.get_vertices()) {
    iter = fmt::format_to(iter, "  {} [{}];\n", vertex_id, std::invoke(vertex_writer, vertex_id, vertex));
  }

  auto const edge_specifier{detail::graph_type_to_edge_specifier(Graph_type)};
  for (auto const& [edge_id, edge] : graph.get_edges()) {
    auto const [source_id, target_id]{edge_id};
    iter = fmt::format_to(
        iter, "  {} {} {} [{}];\n", source_id, edge_specifier, target_id, std::invoke(edge_writer, edge_id, edge));
  }

  iter = fmt::format_to(iter, "}}\n");
}

namespace detail {
  [[nodiscard]] inline auto html_escaped(llvm::StringRef string) -> std::string {
    std::string result;
    result.reserve(
        gsl::narrow_cast<std::size_t>(gsl::narrow_cast<double>(string.size()) * 1.2));  // NOLINT(*magic-number*)
    for (char ch : string) {
      switch (ch) {
        case '&':
          result += "&amp;";
          break;
        case '\"':
          result += "&quot;";
          break;
        case '\'':
          result += "&apos;";
          break;
        case '<':
          result += "&lt;";
          break;
        case '>':
          result += "&gt;";
          break;
        default:
          result += ch;
      }
    }
    return result;
  }
}  // namespace detail

class [[nodiscard]] Reference_writer {
 public:
  [[nodiscard]] auto operator()(graaf::vertex_id_t /*id*/, Reference const& reference) const -> std::string {
    std::string result{};

    // clang-format off
    result += "label=<\n"
              "    <TABLE BORDER = \"0\" CELLBORDER = \"1\" CELLSPACING = \"0\">\n";
    // clang-format on

    result += fmt::format(
        "      <TR>\n"
        "        <TD COLSPAN=\"6\">{}</TD>\n"
        "      </TR>\n",
        magic_enum::enum_name(reference.kind));

    if (reference.kind != SymbolKind::File) {
      result += fmt::format(
          "      <TR>\n"
          "        <TD COLSPAN=\"2\">{}</TD><TD COLSPAN=\"2\">{}</TD><TD COLSPAN=\"2\">{}</TD>\n"
          "      </TR>\n",
          reference.namespace_scopes,
          detail::html_escaped(reference.local_scopes),
          detail::html_escaped(reference.name));
    }

    result += fmt::format(
        "      <TR>\n"
        "        <TD COLSPAN=\"6\">{}</TD>\n"
        "      </TR>\n",
        detail::html_escaped(std::filesystem::relative(reference.uri.file().data(),
                                                       workspace_root.value_or(std::filesystem::current_path()))
                                 .string()));

    if (reference.kind != SymbolKind::File) {
      result += fmt::format(
          "      <TR>\n"
          "        <TD COLSPAN=\"3\">{}</TD><TD COLSPAN=\"3\">{}</TD>\n"
          "      </TR>\n",
          detail::raw_streamed(reference.name_range),
          reference.full_range ? detail::raw_streamed(reference.full_range) : "");
    }

    // clang-format off
    result += "    </TABLE>>,\n"
              "    shape=none";
    // clang-format on
    return result;
  }

  std::optional<std::filesystem::path> workspace_root;  // NOLINT(*non-private-member*)
};
}  // namespace cppcia

#endif