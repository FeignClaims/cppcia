#include "cppcia/test/referencer.hpp"

#include "cppcia/referencer.hpp"
#include "cppcia/test/extractor.hpp"

namespace cppcia {
[[nodiscard]] auto make_referencer_for_test() -> Referencer {
  return Referencer{make_extractor_for_test(), true};
}
}  // namespace cppcia