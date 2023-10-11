#include <boost/ut.hpp>

auto main() -> int {          // NOLINT(bugprone-exception-escape)
  using namespace boost::ut;  // NOLINT(*using-namespace*)

  "placeholder"_test = []() {
    expect(1_c != 2_c);
  };
}