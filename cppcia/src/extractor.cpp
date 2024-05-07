#include "cppcia/extractor.hpp"

#include <concepts>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <clangd/ClangdServer.h>
#include <clangd/GlobalCompilationDatabase.h>
#include <clangd/Hover.h>
#include <clangd/Protocol.h>
#include <clangd/SourceCode.h>
#include <clangd/TUScheduler.h>
#include <clangd/XRefs.h>
#include <clangd/index/Index.h>
#include <clangd/index/MemIndex.h>
#include <clangd/index/Serialization.h>
#include <clangd/index/SymbolOrigin.h>
#include <clangd/support/Logger.h>
#include <clangd/support/Path.h>
#include <clangd/support/Threading.h>
#include <clangd/support/ThreadsafeFS.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <range/v3/all.hpp>

namespace cppcia {
namespace {
  template <typename T>
  [[nodiscard]] auto make_expectedless_callback(clang::clangd::Notification& done,
                                                std::invocable<T&> auto function) -> auto {
    return [&done, function{std::move(function)}](llvm::Expected<T> expected) mutable {
      if (!expected) {
        clang::clangd::elog("{0}", llvm::toString(expected.takeError()));
        return;
      }
      std::invoke(std::move(function), *expected);
      done.notify();
    };
  }

  void append_range(auto& container, auto&& range) {
    for (auto& value : range) {
      container.emplace_back(value);
    }
  }
}  // namespace

Extractor::Extractor(std::unique_ptr<clang::clangd::GlobalCompilationDatabase> cdb,
                     std::unique_ptr<clang::clangd::ThreadsafeFS> tfs,
                     clang::clangd::ClangdServer::Options options,
                     std::unique_ptr<clang::clangd::SymbolIndex> symbol_index)
    : cdb_{std::move(cdb)}, tfs_{std::move(tfs)}, symbol_index_{std::move(symbol_index)} {
  options.StaticIndex = symbol_index_.get();
  server_             = std::make_unique<clang::clangd::ClangdServer>(*cdb_, *tfs_, std::move(options));
}

void Extractor::update_file(clang::clangd::PathRef file, llvm::StringRef content) {
  server_->addDocument(file, content, "null", clang::clangd::WantDiagnostics::No, false);
}

[[nodiscard]] auto Extractor::query_file(llvm::StringRef file) -> std::vector<clang::clangd::DocumentSymbol> {
  std::vector<clang::clangd::DocumentSymbol> result;

  {
    clang::clangd::Notification done;
    server_->documentSymbols(
        file,
        make_expectedless_callback<std::vector<clang::clangd::DocumentSymbol>>(
            done, [&result](std::vector<clang::clangd::DocumentSymbol>& symbols) { result = std::move(symbols); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::query_location_pos(clang::clangd::PathRef file, clang::clangd::Position pos)
    -> std::vector<clang::clangd::LocatedSymbol> {
  std::vector<clang::clangd::LocatedSymbol> result;

  {
    clang::clangd::Notification done;
    server_->locateSymbolAt(
        file,
        pos,
        make_expectedless_callback<std::vector<clang::clangd::LocatedSymbol>>(
            done, [&result](std::vector<clang::clangd::LocatedSymbol>& info) { result = std::move(info); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::query_location_info(clang::clangd::PathRef file, clang::clangd::Position pos)
    -> std::optional<clang::clangd::HoverInfo> {
  std::optional<clang::clangd::HoverInfo> result;

  {
    clang::clangd::Notification done;
    server_->findHover(
        file,
        pos,
        make_expectedless_callback<std::optional<clang::clangd::HoverInfo>>(
            done, [&result](std::optional<clang::clangd::HoverInfo>& info) { result = std::move(info); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::query_name(llvm::StringRef name,
                                         bool fuzzy) -> std::vector<clang::clangd::SymbolInformation> {
  auto [_, unqualified_name]{clang::clangd::splitQualifiedName(name)};

  std::vector<clang::clangd::SymbolInformation> result;

  {
    clang::clangd::Notification done;
    server_->workspaceSymbols(
        name,
        0,
        make_expectedless_callback<std::vector<clang::clangd::SymbolInformation>>(
            done, [&result, unqualified_name, fuzzy](std::vector<clang::clangd::SymbolInformation>& symbols) {
              if (fuzzy) {
                result = std::move(symbols);
              } else {
                append_range(result, symbols | ranges::views::filter([unqualified_name](auto const& symbol) {
                                       return symbol.name == unqualified_name;
                                     }));
              }
            }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::find_type(clang::clangd::PathRef file,
                                        clang::clangd::Position pos) -> std::vector<clang::clangd::LocatedSymbol> {
  std::vector<clang::clangd::LocatedSymbol> result;

  {
    clang::clangd::Notification done;
    server_->findType(
        file,
        pos,
        make_expectedless_callback<std::vector<clang::clangd::LocatedSymbol>>(
            done, [&result](std::vector<clang::clangd::LocatedSymbol>& symbols) { result = std::move(symbols); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::find_references(clang::clangd::PathRef file,
                                              clang::clangd::Position pos) -> clang::clangd::ReferencesResult {
  clang::clangd::ReferencesResult result;

  {
    clang::clangd::Notification done;
    server_->findReferences(file,
                            pos,
                            0,
                            false,
                            make_expectedless_callback<clang::clangd::ReferencesResult>(
                                done, [&result](clang::clangd::ReferencesResult& refs) { result = std::move(refs); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::prepare_call_hierarchy(clang::clangd::PathRef file, clang::clangd::Position pos)
    -> std::vector<clang::clangd::CallHierarchyItem> {
  std::vector<clang::clangd::CallHierarchyItem> result{};

  {
    clang::clangd::Notification done;
    server_->prepareCallHierarchy(
        file,
        pos,
        make_expectedless_callback<std::vector<clang::clangd::CallHierarchyItem>>(
            done, [&result](std::vector<clang::clangd::CallHierarchyItem>& items) { result = std::move(items); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::find_callers(std::vector<clang::clangd::CallHierarchyItem> const& items)
    -> std::vector<clang::clangd::CallHierarchyIncomingCall> {
  std::vector<clang::clangd::CallHierarchyIncomingCall> result;

  for (clang::clangd::Notification done; auto const& item : items) {
    server_->incomingCalls(item,
                           make_expectedless_callback<std::vector<clang::clangd::CallHierarchyIncomingCall>>(
                               done, [&result](std::vector<clang::clangd::CallHierarchyIncomingCall>& callers) {
                                 append_range(result, std::move(callers));
                               }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::prepare_type_hierarchy(clang::clangd::PathRef file, clang::clangd::Position pos)
    -> std::vector<clang::clangd::TypeHierarchyItem> {
  std::vector<clang::clangd::TypeHierarchyItem> result{};

  {
    clang::clangd::Notification done;
    server_->typeHierarchy(
        file,
        pos,
        0,
        clang::clangd::TypeHierarchyDirection::Both,
        make_expectedless_callback<std::vector<clang::clangd::TypeHierarchyItem>>(
            done, [&result](std::vector<clang::clangd::TypeHierarchyItem>& items) { result = std::move(items); }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::find_supertypes(std::vector<clang::clangd::TypeHierarchyItem> const& items)
    -> std::vector<clang::clangd::TypeHierarchyItem> {
  std::vector<clang::clangd::TypeHierarchyItem> result;

  for (clang::clangd::Notification done; auto const& item : items) {
    server_->superTypes(item,
                        make_expectedless_callback<std::optional<std::vector<clang::clangd::TypeHierarchyItem>>>(
                            done, [&result](std::optional<std::vector<clang::clangd::TypeHierarchyItem>>& parents) {
                              if (!parents) {
                                return;
                              }
                              append_range(result, std::move(*parents));
                            }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto Extractor::find_subtypes(std::vector<clang::clangd::TypeHierarchyItem> const& items)
    -> std::vector<clang::clangd::TypeHierarchyItem> {
  std::vector<clang::clangd::TypeHierarchyItem> result{};

  for (clang::clangd::Notification done; auto const& item : items) {
    server_->subTypes(item,
                      make_expectedless_callback<std::vector<clang::clangd::TypeHierarchyItem>>(
                          done, [&result](std::vector<clang::clangd::TypeHierarchyItem>& subtypes) {
                            append_range(result, std::move(subtypes));
                          }));
    done.wait();
  }

  return result;
}

[[nodiscard]] auto read_file(clang::clangd::PathRef file) -> std::string {
  std::ifstream ifile{file.str()};
  std::ostringstream oss{};
  oss << ifile.rdbuf();
  return std::move(oss).str();
}

[[nodiscard]] auto load_index(clang::clangd::PathRef index_file) -> std::unique_ptr<clang::clangd::SymbolIndex> {
  clang::clangd::log("Indexing using the index at {0}.", index_file.str());

  auto symbol_index{std::make_unique<clang::clangd::SwapIndex>(std::make_unique<clang::clangd::MemIndex>())};
  if (auto index{clang::clangd::loadIndex(index_file, clang::clangd::SymbolOrigin::Static, /*UseDex=*/true)}) {
    symbol_index->reset(std::move(index));
  }
  return symbol_index;
}

[[nodiscard]] auto make_extractor(clang::clangd::PathRef index_file,
                                  clang::clangd::PathRef compile_commands_dir,
                                  clang::clangd::PathRef resource_dir,
                                  std::vector<std::string> query_driver_globs) -> Extractor {
  auto tfs{std::make_unique<clang::clangd::RealThreadsafeFS>()};
  auto cdb{std::invoke([&]() {
    clang::clangd::DirectoryBasedGlobalCompilationDatabase::Options cdb_options{*tfs};
    cdb_options.CompileCommandsDir = compile_commands_dir;
    return std::make_unique<clang::clangd::DirectoryBasedGlobalCompilationDatabase>(cdb_options);
  })};
  auto options{std::invoke([&]() {
    clang::clangd::ClangdServer::Options initer{};
    initer.ClangTidyProvider       = nullptr;
    initer.BuildDynamicSymbolIndex = true;
    if (!resource_dir.empty()) {
      initer.ResourceDir = resource_dir;
    }
    initer.QueryDriverGlobs = std::move(query_driver_globs);
    return initer;
  })};

  return Extractor{std::move(cdb), std::move(tfs), std::move(options), load_index(index_file)};
}
}  // namespace cppcia