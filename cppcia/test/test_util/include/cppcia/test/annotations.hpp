#ifndef CPPCIA_TEST_DETAIL_ANNOTATIONS_HPP
#define CPPCIA_TEST_DETAIL_ANNOTATIONS_HPP

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <clangd/Protocol.h>
#include <clangd/support/Path.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Testing/Annotations/Annotations.h>

namespace cppcia {
class Annotations {
 public:
  struct Range {
   public:
    [[nodiscard]] friend auto operator==(Range const& lhs, Range const& rhs) -> bool = default;

    std::size_t begin{};
    std::size_t end{};
  };

  explicit Annotations(llvm::StringRef text);

  [[nodiscard]] auto code() const -> llvm::StringRef {
    return code_;
  }

  [[nodiscard]] auto point(llvm::StringRef name = "") const -> clang::clangd::Position;
  [[nodiscard]] auto point_with_payload(llvm::StringRef name
                                        = "") const -> std::pair<clang::clangd::Position, llvm::StringRef>;
  [[nodiscard]] auto points(llvm::StringRef name = "") const -> std::vector<clang::clangd::Position>;
  [[nodiscard]] auto points_with_payload(llvm::StringRef name = "") const
      -> std::vector<std::pair<clang::clangd::Position, llvm::StringRef>>;

  [[nodiscard]] auto range(llvm::StringRef name = "") const -> clang::clangd::Range;
  [[nodiscard]] auto range_with_payload(llvm::StringRef name
                                        = "") const -> std::pair<clang::clangd::Range, llvm::StringRef>;
  [[nodiscard]] auto ranges(llvm::StringRef name = "") const -> std::vector<clang::clangd::Range>;
  [[nodiscard]] auto ranges_with_payload(llvm::StringRef name
                                         = "") const -> std::vector<std::pair<clang::clangd::Range, llvm::StringRef>>;

 private:
  [[nodiscard]] auto point_with_payload_impl(llvm::StringRef name = "") const -> std::pair<size_t, llvm::StringRef>;
  [[nodiscard]] auto points_impl(llvm::StringRef name = "") const -> std::vector<size_t>;
  [[nodiscard]] auto points_with_payload_impl(llvm::StringRef name
                                              = "") const -> std::vector<std::pair<size_t, llvm::StringRef>>;
  [[nodiscard]] auto range_with_payload_impl(llvm::StringRef name = "") const -> std::pair<Range, llvm::StringRef>;
  [[nodiscard]] auto ranges_impl(llvm::StringRef name = "") const -> std::vector<Range>;
  [[nodiscard]] auto ranges_with_payload_impl(llvm::StringRef name
                                              = "") const -> std::vector<std::pair<Range, llvm::StringRef>>;

  std::string code_;

  struct Annotation {
   public:
    [[nodiscard]] auto is_point() const -> bool {
      return end == std::numeric_limits<std::size_t>::max();
    }

    // NOLINTBEGIN(*non-private-member*)
    std::size_t begin{};
    std::size_t end{std::numeric_limits<std::size_t>::max()};
    llvm::StringRef name;
    llvm::StringRef payload;
    // NOLINTEND(*non-private-member*)
  };

  std::vector<Annotation> annotations_;
  llvm::StringMap<llvm::SmallVector<size_t, 1>> points_;
  llvm::StringMap<llvm::SmallVector<size_t, 1>> ranges_;
};

struct Mock_file {
 public:
  Mock_file(clang::clangd::PathRef path, Annotations annotations);

  [[nodiscard]] auto path() const -> clang::clangd::PathRef {
    return path_;
  }
  [[nodiscard]] auto annotations() const -> Annotations const& {
    return annotations_;
  }

 private:
  std::string path_;
  Annotations annotations_;
};
}  // namespace cppcia

#endif