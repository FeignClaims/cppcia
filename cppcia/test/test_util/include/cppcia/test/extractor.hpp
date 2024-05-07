#ifndef CPPCIA_TEST_DETAIL_TEST_EXTRACTOR_HPP
#define CPPCIA_TEST_DETAIL_TEST_EXTRACTOR_HPP

#include "cppcia/extractor.hpp"

#include <ctime>
#include <string>

#include <clangd/support/Path.h>
#include <clangd/support/ThreadsafeFS.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace cppcia {
class Mock_fs : public clang::clangd::ThreadsafeFS {
 public:
  [[nodiscard]] auto viewImpl() const -> llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> override;

  llvm::StringMap<std::string> files;                 // NOLINT(*non-private-member*)
  llvm::StringMap<std::time_t> time_stamps;           // NOLINT(*non-private-member*)
  bool overlay_real_file_system_for_modules = false;  // NOLINT(*non-private-member*)
};

auto test_root() -> char const*;
auto test_path(clang::clangd::PathRef file, llvm::sys::path::Style = llvm::sys::path::Style::native) -> std::string;

[[nodiscard]] auto make_extractor_for_test() -> cppcia::Extractor;
}  // namespace cppcia

#endif