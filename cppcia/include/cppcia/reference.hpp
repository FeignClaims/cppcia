#ifndef CPPCIA_REFERENCE_HPP
#define CPPCIA_REFERENCE_HPP

#include "cppcia/detail/hash_value.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <clangd/Protocol.h>
#include <clangd/support/Path.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <graaflib/graph.h>
#include <llvm/ADT/StringRef.h>

namespace cppcia {
using clang::clangd::Location;
using clang::clangd::Range;
using clang::clangd::SymbolKind;
using clang::clangd::URIForFile;

struct Reference {
 public:
  [[nodiscard]] friend auto operator==(Reference const& lhs, Reference const& rhs) -> bool {
    return std::tie(lhs.kind, lhs.uri, lhs.name_range, lhs.namespace_scopes, lhs.local_scopes, lhs.name)
           == std::tie(rhs.kind, rhs.uri, rhs.name_range, rhs.namespace_scopes, rhs.local_scopes, rhs.name);
  }

  [[nodiscard]] auto contains(Reference const& reference) const -> bool;

  // NOLINTBEGIN(*non-private-member*)
  SymbolKind kind;
  URIForFile uri;
  Range name_range;
  std::optional<Range> full_range;
  std::string namespace_scopes;
  std::string local_scopes;
  std::string name;
  // NOLINTEND(*non-private-member*)
};
}  // namespace cppcia

template <>
struct fmt::formatter<cppcia::Reference> : fmt::ostream_formatter {};

template <>
struct std::hash<cppcia::Reference> {
 public:
  [[nodiscard]] auto operator()(cppcia::Reference const& reference) const -> std::size_t {
    return cppcia::detail::hash_value(reference.kind,
                                      reference.uri.file().str(),
                                      reference.name_range.start.line,
                                      reference.name_range.start.character,
                                      reference.name_range.end.line,
                                      reference.name_range.end.character,
                                      reference.namespace_scopes,
                                      reference.local_scopes,
                                      reference.name);
  }
};

namespace cppcia {
[[nodiscard]] auto make_file_reference(llvm::StringRef file) -> Reference;
[[nodiscard]] auto make_file_reference(URIForFile uri) -> Reference;

[[nodiscard]] auto to_file_pos(URIForFile const& uri,
                               Range const& range) -> std::pair<std::string, clang::clangd::Position>;
[[nodiscard]] auto to_file_pos(Location const& location) -> std::pair<std::string, clang::clangd::Position>;
[[nodiscard]] auto to_file_pos(Reference const& reference) -> std::pair<std::string, clang::clangd::Position>;

struct Reference_tree {
  Reference reference;
  std::vector<Reference_tree> children;
};

void visit(std::convertible_to<Reference_tree> auto&& tree,
           std::invocable<Reference> auto&& function) {  // NOLINT(*recursion*)
  std::invoke(function, tree.reference);
  for (auto& child : tree.children) {
    visit(child, function);
  }
}

enum class Edge_type : std::uint8_t { solid, dashed, dotted };

using Reference_graph = graaf::directed_graph<Reference, Edge_type>;

[[nodiscard]] auto to_graph(Reference_tree const& tree, Edge_type edge_type, bool reverse_edge) -> Reference_graph;
}  // namespace cppcia

#endif