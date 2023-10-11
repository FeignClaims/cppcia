#include <boost/ut.hpp>  // import boost.ut;

[[nodiscard]] constexpr auto sum(auto... values) -> auto {
  return (values + ...);
}

int main() {
  using namespace boost::ut;  // NOLINT(*using-namespace*)

  "sum"_test = []() {
    expect(sum(0) == 0_i);
    expect(sum(1, 2) == 3_i);
    expect(sum(1, 2) > 0_i);
  };
}