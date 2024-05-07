#ifndef CPPCIA_EXTRACTOR_HPP
#define CPPCIA_EXTRACTOR_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <clangd/ClangdServer.h>
#include <clangd/GlobalCompilationDatabase.h>
#include <clangd/Hover.h>
#include <clangd/Protocol.h>
#include <clangd/XRefs.h>
#include <clangd/index/Index.h>
#include <clangd/support/Path.h>
#include <clangd/support/ThreadsafeFS.h>
#include <llvm/ADT/StringRef.h>

namespace cppcia {
class Extractor {
 public:
  Extractor(std::unique_ptr<clang::clangd::GlobalCompilationDatabase> cdb,
            std::unique_ptr<clang::clangd::ThreadsafeFS> tfs,
            clang::clangd::ClangdServer::Options options,
            std::unique_ptr<clang::clangd::SymbolIndex> symbol_index = nullptr);

  void update_file(clang::clangd::PathRef file, llvm::StringRef content);

  [[nodiscard]] auto query_file(llvm::StringRef file) -> std::vector<clang::clangd::DocumentSymbol>;
  [[nodiscard]] auto query_location_pos(clang::clangd::PathRef file,
                                        clang::clangd::Position pos) -> std::vector<clang::clangd::LocatedSymbol>;
  [[nodiscard]] auto query_location_info(clang::clangd::PathRef file,
                                         clang::clangd::Position pos) -> std::optional<clang::clangd::HoverInfo>;
  [[nodiscard]] auto query_name(llvm::StringRef name,
                                bool fuzzy = false) -> std::vector<clang::clangd::SymbolInformation>;

  [[nodiscard]] auto find_type(clang::clangd::PathRef file,
                               clang::clangd::Position pos) -> std::vector<clang::clangd::LocatedSymbol>;
  [[nodiscard]] auto find_references(clang::clangd::PathRef file,
                                     clang::clangd::Position pos) -> clang::clangd::ReferencesResult;

  [[nodiscard]] auto prepare_call_hierarchy(clang::clangd::PathRef file, clang::clangd::Position pos)
      -> std::vector<clang::clangd::CallHierarchyItem>;
  [[nodiscard]] auto find_callers(std::vector<clang::clangd::CallHierarchyItem> const& items)
      -> std::vector<clang::clangd::CallHierarchyIncomingCall>;

  [[nodiscard]] auto prepare_type_hierarchy(clang::clangd::PathRef file, clang::clangd::Position pos)
      -> std::vector<clang::clangd::TypeHierarchyItem>;
  [[nodiscard]] auto find_supertypes(std::vector<clang::clangd::TypeHierarchyItem> const& items)
      -> std::vector<clang::clangd::TypeHierarchyItem>;
  [[nodiscard]] auto find_subtypes(std::vector<clang::clangd::TypeHierarchyItem> const& items)
      -> std::vector<clang::clangd::TypeHierarchyItem>;

 private:
  std::unique_ptr<clang::clangd::GlobalCompilationDatabase> cdb_;
  std::unique_ptr<clang::clangd::ThreadsafeFS> tfs_;
  std::unique_ptr<clang::clangd::SymbolIndex> symbol_index_;
  std::unique_ptr<clang::clangd::ClangdServer> server_;
};

[[nodiscard]] auto read_file(clang::clangd::PathRef file) -> std::string;

[[nodiscard]] auto load_index(clang::clangd::PathRef index_file) -> std::unique_ptr<clang::clangd::SymbolIndex>;

[[nodiscard]] auto make_extractor(clang::clangd::PathRef index_file,
                                  clang::clangd::PathRef compile_commands_dir,
                                  clang::clangd::PathRef resource_dir         = {},
                                  std::vector<std::string> query_driver_globs = {}) -> Extractor;
}  // namespace cppcia

#endif
