#ifndef CPPCIA_REFERENCER_HPP
#define CPPCIA_REFERENCER_HPP

#include "cppcia/extractor.hpp"
#include "cppcia/reference.hpp"

#include <optional>
#include <utility>
#include <vector>

#include <clangd/Protocol.h>
#include <clangd/support/Path.h>
#include <graaflib/graph.h>
#include <llvm/ADT/StringRef.h>

namespace cppcia {
class Referencer {
 public:
  explicit Referencer(Extractor extractor, bool for_test = false)
      : extractor_{std::move(extractor)}, for_test_{for_test} {}

  void update_file(clang::clangd::PathRef file, llvm::StringRef content);

  [[nodiscard]] auto query_file(clang::clangd::PathRef file) -> Reference_tree;
  [[nodiscard]] auto query_location(clang::clangd::PathRef file,
                                    clang::clangd::Position pos) -> std::optional<Reference>;
  [[nodiscard]] auto query_location(Location const& location) -> std::optional<Reference> {
    auto [file, pos]{to_file_pos(location)};
    return query_location(file, pos);
  }
  [[nodiscard]] auto query_name(llvm::StringRef name, bool fuzzy = false) -> std::vector<Reference>;

  [[nodiscard]] auto find_container(Reference const& reference) -> Reference;
  [[nodiscard]] auto find_container_path(Reference const& reference) -> Reference_tree;
  [[nodiscard]] auto find_type(Reference const& reference) -> Reference;
  [[nodiscard]] auto find_preferred_declaration(Reference const& reference) -> std::optional<Reference>;
  [[nodiscard]] auto find_references(Reference const& reference) -> Reference_tree;
  [[nodiscard]] auto find_direct_callers(Reference const& reference) -> std::vector<Reference>;
  [[nodiscard]] auto find_caller_hierarchies(Reference const& reference) -> Reference_tree;
  [[nodiscard]] auto find_direct_supertypes(Reference const& reference) -> std::vector<Reference>;
  [[nodiscard]] auto find_supertype_hierarchies(Reference const& reference) -> Reference_tree;
  [[nodiscard]] auto find_direct_subtypes(Reference const& reference) -> std::vector<Reference>;
  [[nodiscard]] auto find_subtype_hierarchies(Reference const& reference) -> Reference_tree;

  [[nodiscard]] auto extractor() -> Extractor& {
    return extractor_;
  }

  [[nodiscard]] auto extractor() const -> Extractor const& {
    return extractor_;
  }

 private:
  void update_real_file_or_test(clang::clangd::PathRef file) {
    if (!for_test_) {
      update_file(file, read_file(file));
    }
  }

  Extractor extractor_;
  bool for_test_;
};

[[nodiscard]] inline auto to_reference(Referencer& referencer,
                                       clang::clangd::PathRef file,
                                       clang::clangd::DocumentSymbol const& symbol) -> Reference {
  auto result{*referencer.query_location(file, symbol.selectionRange.start)};
  result.full_range = symbol.range;
  return result;
}

[[nodiscard]] inline auto to_reference(Referencer& referencer,
                                       clang::clangd::CallHierarchyItem const& item) -> Reference {
  auto result{*referencer.query_location(item.uri.file(), item.selectionRange.start)};
  result.full_range = item.range;
  return result;
}

[[nodiscard]] inline auto to_reference(Referencer& referencer,
                                       clang::clangd::TypeHierarchyItem const& item) -> Reference {
  auto result{*referencer.query_location(item.uri.file(), item.selectionRange.start)};
  result.full_range = item.range;
  return result;
}
}  // namespace cppcia

#endif