#ifndef CPPCIA_DETAIL_HASH_VALUE_HPP
#define CPPCIA_DETAIL_HASH_VALUE_HPP

#include <concepts>
#include <cstddef>
#include <functional>

namespace cppcia {
namespace detail {
  template <typename T>
  concept can_hash = requires(T value) {
                       { std::hash<T>{}(value) } -> std::same_as<std::size_t>;
                     };

  class [[nodiscard]] Hash_value_fn {
   public:
    template <can_hash... Args>
    [[nodiscard]] constexpr auto operator()(Args const&... args) const -> std::size_t {
      std::size_t seed{};
      (..., (seed ^= std::hash<Args>{}(args) + offset + (seed << left_shifts) + (seed >> right_shifts)));
      return seed;
    }

   private:
    static constexpr std::size_t offset{0x9E'37'79'B9};
    static constexpr std::size_t left_shifts{6};
    static constexpr std::size_t right_shifts{2};
  };

  inline namespace cpo {
    inline constexpr auto hash_value{detail::Hash_value_fn{}};
  }  // namespace cpo
}  // namespace detail
}  // namespace cppcia

#endif