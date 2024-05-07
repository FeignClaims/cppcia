#include "cppcia/referencer.hpp"

#include "cppcia/extractor.hpp"
#include "cppcia/reference.hpp"

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

#include <clangd/Hover.h>
#include <clangd/Protocol.h>
#include <clangd/XRefs.h>
#include <clangd/support/Path.h>
#include <fmt/core.h>
#include <llvm/ADT/StringRef.h>
#include <range/v3/all.hpp>

namespace cppcia {
void Referencer::update_file(clang::clangd::PathRef file, llvm::StringRef content) {
  extractor_.update_file(file, content);
}

namespace {
  void query_file_impl(Referencer& referencer,  // NOLINT(*recursion*)
                       Reference_tree& result,
                       clang::clangd::PathRef file,
                       clang::clangd::DocumentSymbol const& symbol) {
    result.reference = to_reference(referencer, file, symbol);
    for (auto const& symbol_child : symbol.children) {
      Reference_tree result_child;
      query_file_impl(referencer, result_child, file, symbol_child);
      result.children.emplace_back(std::move(result_child));
    }
  }
}  // namespace

[[nodiscard]] auto Referencer::query_file(clang::clangd::PathRef file) -> Reference_tree {
  update_real_file_or_test(file);
  std::vector<clang::clangd::DocumentSymbol> symbols{extractor_.query_file(file)};
  return {make_file_reference(file),
          symbols | ranges::views::transform([&](clang::clangd::DocumentSymbol const& symbol) {
            Reference_tree result;
            query_file_impl(*this, result, file, symbol);
            return result;
          }) | ranges::to<std::vector>()};
}

[[nodiscard]] auto Referencer::query_location(clang::clangd::PathRef file,
                                              clang::clangd::Position pos) -> std::optional<Reference> {
  update_real_file_or_test(file);
  std::optional<clang::clangd::HoverInfo> info{extractor_.query_location_info(file, pos)};
  if (!info) {
    return std::nullopt;
  }
  return Reference{.kind{clang::clangd::indexSymbolKindToSymbolKind(info->Kind)},
                   .uri{clang::clangd::URIForFile::canonicalize(file, file)},
                   .name_range{*info->SymRange},
                   .full_range{},
                   .namespace_scopes{std::move(*info->NamespaceScope)},
                   .local_scopes{std::move(info->LocalScope)},
                   .name{std::move(info->Name)}};
}

[[nodiscard]] auto Referencer::query_name(llvm::StringRef name, bool fuzzy) -> std::vector<Reference> {
  std::vector<clang::clangd::SymbolInformation> symbols{extractor_.query_name(name, fuzzy)};

  std::vector<Reference> result{};
  for (auto const& symbol : symbols) {
    std::optional<Reference> reference{query_location(symbol.location.uri.file(), symbol.location.range.start)};
    if (reference) {
      result.emplace_back(*reference);
    }
  }
  return result;
}

[[nodiscard]] auto Referencer::find_container(Reference const& reference) -> Reference {
  Reference_tree tree{query_file(reference.uri.file())};
  while (true) {
    auto iter{ranges::find_if(tree.children, [&reference](auto const& child) {
      return child.reference.contains(reference) && child.reference != reference;
    })};
    if (iter == tree.children.end()) {
      break;
    }
    tree = std::move(*iter);
  }
  return tree.reference;
}

[[nodiscard]] auto Referencer::find_container_path(Reference const& reference) -> Reference_tree {
  Reference_tree tree{query_file(reference.uri.file())};

  for (Reference_tree* current{&tree}; true;) {
    auto iter{ranges::find_if(current->children,
                              [&reference](auto const& child) { return child.reference.contains(reference); })};
    if (iter == current->children.end()) {
      break;
    }
    current->children = std::vector{std::move(*iter)};
    current           = &current->children.front();
  }

  return tree;
}

[[nodiscard]] auto Referencer::find_type(Reference const& reference) -> Reference {
  auto [file, pos]{to_file_pos(reference)};
  Location preferred_location{extractor_.find_type(file, pos).front().PreferredDeclaration};
  return *query_location(preferred_location);
}

[[nodiscard]] auto Referencer::find_preferred_declaration(Reference const& reference) -> std::optional<Reference> {
  auto [file, pos]{to_file_pos(reference)};
  auto symbols{extractor_.query_location_pos(file, pos)};
  if (symbols.empty()) {
    return std::nullopt;
  }
  Location preferred_location{symbols.front().PreferredDeclaration};
  return query_location(preferred_location);
}

[[nodiscard]] auto Referencer::find_references(Reference const& reference) -> Reference_tree {
  Reference root{find_preferred_declaration(reference).value_or(reference)};

  auto [file, pos]{to_file_pos(reference)};
  clang::clangd::ReferencesResult references{extractor_.find_references(file, pos)};

  return Reference_tree{
      root,
      // clang-format off
      references.References
          | ranges::views::transform([](clang::clangd::ReferencesResult::Reference const& value) {
              return static_cast<Location>(value.Loc);  // NOLINT(*slicing*)
            })
          | ranges::views::filter([&](Location const& location) {
              return !(root.uri == location.uri && root.name_range == location.range);
            })
          | ranges::views::transform([this](Location const& location) { return Reference_tree{*query_location(location), {}}; })
          | ranges::to<std::vector>()};
  // clang-format on
}

[[nodiscard]] auto Referencer::find_direct_callers(Reference const& reference) -> std::vector<Reference> {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::CallHierarchyIncomingCall> callers{
      extractor_.find_callers(extractor_.prepare_call_hierarchy(file, pos))};
  // clang-format off
  return callers
         | ranges::views::transform([this](clang::clangd::CallHierarchyIncomingCall const& caller) {
             return to_reference(*this, caller.from);
           })
         | ranges::to<std::vector>();
  // clang-format on
}

namespace {
  void find_caller_hierarchies_impl(Referencer& referencer,  //  NOLINT(*recursion*)
                                    Reference_tree& result,
                                    clang::clangd::CallHierarchyItem item) {
    result.reference = to_reference(referencer, item);
    for (auto caller : referencer.extractor().find_callers(std::vector{std::move(item)})) {
      Reference_tree child;
      find_caller_hierarchies_impl(referencer, child, std::move(caller.from));
      result.children.emplace_back(std::move(child));
    }
  }
}  // namespace

[[nodiscard]] auto Referencer::find_caller_hierarchies(Reference const& reference) -> Reference_tree {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::CallHierarchyItem> items{extractor_.prepare_call_hierarchy(file, pos)};
  assert(!items.empty());

  Reference_tree result;
  find_caller_hierarchies_impl(*this, result, items.front());
  return result;
}

[[nodiscard]] auto Referencer::find_direct_supertypes(Reference const& reference) -> std::vector<Reference> {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::TypeHierarchyItem> supertypes{
      extractor_.find_supertypes(extractor_.prepare_type_hierarchy(file, pos))};
  // clang-format off
  return supertypes
         | ranges::views::transform([this](clang::clangd::TypeHierarchyItem const& supertype) {
             return to_reference(*this, supertype);
           })
         | ranges::to<std::vector>();
  // clang-format on
}

namespace {
  void find_supertypes_impl(Referencer& referencer,  //  NOLINT(*recursion*)
                            Reference_tree& result,
                            clang::clangd::TypeHierarchyItem item) {
    result.reference = to_reference(referencer, item);
    for (auto subtype : referencer.extractor().find_supertypes(std::vector{std::move(item)})) {
      Reference_tree child;
      find_supertypes_impl(referencer, child, std::move(subtype));
      result.children.emplace_back(std::move(child));
    }
  }
}  // namespace

[[nodiscard]] auto Referencer::find_supertype_hierarchies(Reference const& reference) -> Reference_tree {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::TypeHierarchyItem> items{extractor_.prepare_type_hierarchy(file, pos)};
  assert(!items.empty());

  Reference_tree result;
  find_supertypes_impl(*this, result, items.front());
  return result;
}

[[nodiscard]] auto Referencer::find_direct_subtypes(Reference const& reference) -> std::vector<Reference> {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::TypeHierarchyItem> supertypes{
      extractor_.find_subtypes(extractor_.prepare_type_hierarchy(file, pos))};
  // clang-format off
  return supertypes
         | ranges::views::transform([this](clang::clangd::TypeHierarchyItem const& subtypes) {
             return to_reference(*this, subtypes);
           })
         | ranges::to<std::vector>();
  // clang-format on
}

namespace {
  void find_subtypes_impl(Referencer& referencer,  //  NOLINT(*recursion*)
                          Reference_tree& result,
                          clang::clangd::TypeHierarchyItem item) {
    result.reference = to_reference(referencer, item);
    for (auto subtype : referencer.extractor().find_subtypes(std::vector{std::move(item)})) {
      Reference_tree child;
      find_subtypes_impl(referencer, child, std::move(subtype));
      result.children.emplace_back(std::move(child));
    }
  }
}  // namespace

[[nodiscard]] auto Referencer::find_subtype_hierarchies(Reference const& reference) -> Reference_tree {
  auto [file, pos]{to_file_pos(*find_preferred_declaration(reference))};
  std::vector<clang::clangd::TypeHierarchyItem> items{extractor_.prepare_type_hierarchy(file, pos)};
  assert(!items.empty());

  Reference_tree result;
  find_subtypes_impl(*this, result, items.front());
  return result;
}
}  // namespace cppcia