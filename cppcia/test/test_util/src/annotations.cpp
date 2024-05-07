#include "cppcia/test/annotations.hpp"

#include "cppcia/test/extractor.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include <clangd/Protocol.h>
#include <clangd/SourceCode.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Testing/Annotations/Annotations.h>
#include <support/Path.h>

namespace cppcia {
namespace {
  void require(bool assertion, char const* message, llvm::StringRef code) {
    if (!assertion) {
      llvm::errs() << "Annotated testcase: " << message << "\n" << code << "\n";
      llvm_unreachable("Annotated testcase assertion failed!");
    }
  }
}  // namespace

Annotations::Annotations(llvm::StringRef text) {
  auto const text_require{[text](bool assertion, char const* msg) {
    require(assertion, msg, text);
  }};
  std::optional<llvm::StringRef> name;
  std::optional<llvm::StringRef> payload;
  llvm::SmallVector<Annotation, 8> open_ranges;  // NOLINT(*magic-number*)

  code_.reserve(text.size());
  while (!text.empty()) {
    if (text.consume_front("^")) {
      annotations_.push_back({code_.size(), static_cast<std::size_t>(-1), name.value_or(""), payload.value_or("")});
      points_[name.value_or("")].push_back(annotations_.size() - 1);
      name    = std::nullopt;
      payload = std::nullopt;
      continue;
    }
    if (text.consume_front("[[")) {
      open_ranges.push_back({code_.size(), static_cast<std::size_t>(-1), name.value_or(""), payload.value_or("")});
      name    = std::nullopt;
      payload = std::nullopt;
      continue;
    }
    text_require(!name, "$name should be followed by ^ or [[");
    if (text.consume_front("]]")) {
      text_require(!open_ranges.empty(), "unmatched ]]");

      Annotation const& new_range{open_ranges.back()};
      annotations_.push_back({new_range.begin, code_.size(), new_range.name, new_range.payload});
      ranges_[new_range.name].push_back(annotations_.size() - 1);

      open_ranges.pop_back();
      continue;
    }
    if (text.consume_front("$")) {
      name = text.take_while([](char ch) { return llvm::isAlnum(ch) || ch == '_'; });
      text = text.drop_front(name->size());

      if (text.consume_front("(")) {
        payload = text.take_while([](char ch) { return ch != ')'; });
        text_require(text.size() > payload->size(), "unterminated payload");
        text = text.drop_front(payload->size() + 1);
      }

      continue;
    }
    code_.push_back(text.front());
    text = text.drop_front();
  }
  text_require(!name, "unterminated $name");
  text_require(open_ranges.empty(), "unmatched [[");
}

auto Annotations::point(llvm::StringRef name) const -> clang::clangd::Position {
  return point_with_payload(name).first;
}

auto Annotations::point_with_payload(llvm::StringRef name) const
    -> std::pair<clang::clangd::Position, llvm::StringRef> {
  auto [base_point, payload]{point_with_payload_impl(name)};
  return {clang::clangd::offsetToPosition(code(), base_point), payload};
}

auto Annotations::points(llvm::StringRef name) const -> std::vector<clang::clangd::Position> {
  auto base_points{points_impl(name)};

  std::vector<clang::clangd::Position> positions;
  positions.reserve(base_points.size());
  for (auto const point : base_points) {
    positions.push_back(clang::clangd::offsetToPosition(code(), point));
  }

  return positions;
}

auto Annotations::points_with_payload(llvm::StringRef name) const
    -> std::vector<std::pair<clang::clangd::Position, llvm::StringRef>> {
  auto base_points{points_with_payload_impl(name)};

  std::vector<std::pair<clang::clangd::Position, llvm::StringRef>> positions;
  positions.reserve(base_points.size());
  for (auto const& [point, payload] : base_points) {
    positions.emplace_back(clang::clangd::offsetToPosition(code(), point), payload);
  }

  return positions;
}

namespace {
  auto to_lsp_range(llvm::StringRef code, Annotations::Range range) -> clang::clangd::Range {
    clang::clangd::Range lsp_range;
    lsp_range.start = clang::clangd::offsetToPosition(code, range.begin);
    lsp_range.end   = clang::clangd::offsetToPosition(code, range.end);
    return lsp_range;
  }
}  // namespace

auto Annotations::range(llvm::StringRef name) const -> clang::clangd::Range {
  return range_with_payload(name).first;
}

auto Annotations::range_with_payload(llvm::StringRef name) const -> std::pair<clang::clangd::Range, llvm::StringRef> {
  auto [base_range, payload]{range_with_payload_impl(name)};
  return {to_lsp_range(code(), base_range), payload};
}

auto Annotations::ranges(llvm::StringRef name) const -> std::vector<clang::clangd::Range> {
  auto offset_ranges{ranges_impl(name)};

  std::vector<clang::clangd::Range> ranges;
  ranges.reserve(offset_ranges.size());
  for (auto const& range : offset_ranges) {
    ranges.push_back(to_lsp_range(code(), range));
  }

  return ranges;
}

auto Annotations::ranges_with_payload(llvm::StringRef name) const
    -> std::vector<std::pair<clang::clangd::Range, llvm::StringRef>> {
  auto offset_ranges{ranges_with_payload_impl(name)};

  std::vector<std::pair<clang::clangd::Range, llvm::StringRef>> ranges;
  ranges.reserve(offset_ranges.size());
  for (auto const& [range, payload] : offset_ranges) {
    ranges.emplace_back(to_lsp_range(code(), range), payload);
  }

  return ranges;
}

auto Annotations::point_with_payload_impl(llvm::StringRef name) const -> std::pair<std::size_t, llvm::StringRef> {
  auto i{points_.find(name)};
  require(i != points_.end() && i->getValue().size() == 1, "expected exactly one point", code_);
  Annotation const& point = annotations_[i->getValue()[0]];
  return {point.begin, point.payload};
}

auto Annotations::points_impl(llvm::StringRef name) const -> std::vector<std::size_t> {
  auto pts{points_with_payload_impl(name)};
  std::vector<std::size_t> positions;
  positions.reserve(pts.size());
  for (auto const& [point, payload] : pts) {
    positions.push_back(point);
  }
  return positions;
}

auto Annotations::points_with_payload_impl(llvm::StringRef name) const
    -> std::vector<std::pair<std::size_t, llvm::StringRef>> {
  auto iter{points_.find(name)};
  if (iter == points_.end()) {
    return {};
  }

  std::vector<std::pair<std::size_t, llvm::StringRef>> res;
  res.reserve(iter->getValue().size());
  for (std::size_t const i : iter->getValue()) {
    res.emplace_back(annotations_[i].begin, annotations_[i].payload);
  }

  return res;
}

auto Annotations::range_with_payload_impl(llvm::StringRef name) const
    -> std::pair<Annotations::Range, llvm::StringRef> {
  auto i{ranges_.find(name)};
  require(i != ranges_.end() && i->getValue().size() == 1, "expected exactly one range", code_);
  Annotation const& range{annotations_[i->getValue()[0]]};
  return {{range.begin, range.end}, range.payload};
}

std::vector<Annotations::Range> Annotations::ranges_impl(llvm::StringRef name) const {
  auto with_payload{ranges_with_payload_impl(name)};
  std::vector<Annotations::Range> res;
  res.reserve(with_payload.size());
  for (auto const& [range, payload] : with_payload) {
    res.push_back(range);
  }
  return res;
}

auto Annotations::ranges_with_payload_impl(llvm::StringRef name) const
    -> std::vector<std::pair<Annotations::Range, llvm::StringRef>> {
  auto iter{ranges_.find(name)};
  if (iter == ranges_.end()) {
    return {};
  }

  std::vector<std::pair<Annotations::Range, llvm::StringRef>> res;
  res.reserve(iter->getValue().size());
  for (std::size_t const i : iter->getValue()) {
    res.emplace_back(Annotations::Range{annotations_[i].begin, annotations_[i].end}, annotations_[i].payload);
  }

  return res;
}

Mock_file::Mock_file(clang::clangd::PathRef path, Annotations annotations)
    : path_{test_path(path)}, annotations_{std::move(annotations)} {}
}  // namespace cppcia