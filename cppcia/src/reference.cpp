#include "cppcia/reference.hpp"

#include <queue>
#include <string>
#include <utility>

#include <Protocol.h>
#include <fmt/core.h>
#include <graaflib/graph.h>
#include <graaflib/types.h>
#include <llvm/ADT/StringRef.h>
#include <magic_enum/magic_enum.hpp>

namespace cppcia {
[[nodiscard]] auto Reference::contains(Reference const& reference) const -> bool {
  return uri == reference.uri
         && (kind == SymbolKind::File || full_range.value_or(name_range).contains(reference.name_range));
}

[[nodiscard]] auto make_file_reference(llvm::StringRef file) -> Reference {
  return Reference{.kind{SymbolKind::File},
                   .uri{clang::clangd::URIForFile::canonicalize(file, file)},
                   .name_range{},
                   .full_range{},
                   .namespace_scopes{},
                   .local_scopes{},
                   .name{}};
}

[[nodiscard]] auto make_file_reference(URIForFile const& uri) -> Reference {
  return make_file_reference(uri.file());
}

[[nodiscard]] auto to_file_pos(URIForFile const& uri,
                               Range const& range) -> std::pair<std::string, clang::clangd::Position> {
  return std::pair<std::string, clang::clangd::Position>{uri.file(), range.start};
}

[[nodiscard]] auto to_file_pos(Location const& location) -> std::pair<std::string, clang::clangd::Position> {
  return to_file_pos(location.uri, location.range);
}

[[nodiscard]] auto to_file_pos(Reference const& reference) -> std::pair<std::string, clang::clangd::Position> {
  return to_file_pos(reference.uri, reference.name_range);
}

[[nodiscard]] auto to_graph(Reference_tree const& tree, Edge_type edge_type, bool reverse_edge) -> Reference_graph {
  Reference_graph result;

  struct Value_type {
    Reference_tree const* reference_tree;
    graaf::vertex_id_t id;
  };

  std::queue<Value_type> queue;
  {
    auto id{result.add_vertex(tree.reference)};
    queue.emplace(&tree, id);
  }
  while (!queue.empty()) {
    Value_type current{queue.front()};
    queue.pop();

    for (auto const& child : current.reference_tree->children) {
      auto child_id{result.add_vertex(child.reference)};
      if (reverse_edge) {
        result.add_edge(child_id, current.id, edge_type);
      } else {
        result.add_edge(current.id, child_id, edge_type);
      }
      queue.emplace(&child, child_id);
    }
  }

  return result;
}
}  // namespace cppcia