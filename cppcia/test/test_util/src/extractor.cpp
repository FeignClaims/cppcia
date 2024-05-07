#include "cppcia/extractor.hpp"

#include "cppcia/test/extractor.hpp"

#include <cassert>
#include <ctime>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <clangd/ClangdServer.h>
#include <clangd/GlobalCompilationDatabase.h>
#include <clangd/TUScheduler.h>
#include <clangd/support/Path.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace cppcia {
namespace {
  auto build_test_fs(llvm::StringMap<std::string> const& files, llvm::StringMap<std::time_t> const& time_stamps)
      -> llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> {
    llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> mem_fs(new llvm::vfs::InMemoryFileSystem);
    std::ignore = mem_fs->setCurrentWorkingDirectory(test_root());
    for (auto& file_and_contents : files) {
      llvm::StringRef const file = file_and_contents.first();
      mem_fs->addFile(
          file, time_stamps.lookup(file), llvm::MemoryBuffer::getMemBufferCopy(file_and_contents.second, file));
    }
    return mem_fs;
  }
}  // namespace

[[nodiscard]] auto Mock_fs::viewImpl() const -> llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> {
  auto mem_fs = build_test_fs(files, time_stamps);
  if (!overlay_real_file_system_for_modules) {
    return mem_fs;
  }
  llvm::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> overlay_file_system
      = new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem());
  overlay_file_system->pushOverlay(mem_fs);
  return overlay_file_system;
}

auto test_root() -> char const* {
#ifdef _WIN32
  return "C:\\clangd-test";
#else
  return "/clangd-test";
#endif
}

auto test_path(clang::clangd::PathRef file, llvm::sys::path::Style style) -> std::string {
  assert(llvm::sys::path::is_relative(file) && "FileName should be relative");

  llvm::SmallString<32> native_file = file;  // NOLINT(*magic-number*)
  llvm::sys::path::native(native_file, style);
  llvm::SmallString<32> path;  // NOLINT(*magic-number*)
  llvm::sys::path::append(path, style, test_root(), native_file);
  return std::string(path.str());
}

[[nodiscard]] auto make_extractor_for_test() -> cppcia::Extractor {
  auto fs{std::make_unique<Mock_fs>()};
  clang::clangd::DirectoryBasedGlobalCompilationDatabase::Options cdb_opts(*fs);

  clang::clangd::ClangdServer::Options options{};
  options.UpdateDebounce          = clang::clangd::DebouncePolicy::fixed({});
  options.StorePreamblesInMemory  = true;
  options.AsyncThreadsCount       = 4;
  options.ClangTidyProvider       = nullptr;
  options.BuildDynamicSymbolIndex = true;
  return {std::make_unique<clang::clangd::DirectoryBasedGlobalCompilationDatabase>(std::move(cdb_opts)),
          std::move(fs),
          std::move(options)};
}
}  // namespace cppcia