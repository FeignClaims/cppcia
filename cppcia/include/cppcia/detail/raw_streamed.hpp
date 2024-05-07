#ifndef CPPCIA_DETAIL_RAW_STREAMED_HPP
#define CPPCIA_DETAIL_RAW_STREAMED_HPP

#include <string>

#include <llvm/Support/raw_ostream.h>

namespace cppcia::detail {
template <typename T>
  requires requires(llvm::raw_ostream& ostream, T t) { ostream << t; }
[[nodiscard]] auto raw_streamed(T&& object) -> std::string {
  std::string result{};
  llvm::raw_string_ostream ostream{result};
  ostream << std::forward<T>(object);
  return result;
}
}  // namespace cppcia::detail

#endif