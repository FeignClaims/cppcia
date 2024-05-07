#ifndef CPPCIA_CPPCIA_CPPCIA_MAIN_HPP
#define CPPCIA_CPPCIA_CPPCIA_MAIN_HPP

#include <gsl/gsl>

namespace cppcia {
[[nodiscard]] auto cppcia_main(int argc, gsl::czstring argv[]) noexcept -> int;  // NOLINT(*c-array*)
}  // namespace cppcia

#endif