#include "cppcia/extractor.hpp"

#include "cppcia/test/annotations.hpp"
#include "cppcia/test/extractor.hpp"

#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <clangd/ClangdServer.h>
#include <clangd/Protocol.h>
#include <clangd/support/Logger.h>
#include <clangd/support/Path.h>
#include <clangd/support/ThreadsafeFS.h>
#include <magic_enum/magic_enum.hpp>

namespace cppcia {
TEST_CASE("query_name", "[extractor]") {
  Extractor extractor{make_extractor_for_test()};
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          int value1    = 0;
                                          double value2 = 10;
                                        )cpp"}};
  extractor.update_file(file.path(), file.annotations().code());

  std::vector<clang::clangd::DocumentSymbol> symbols{extractor.query_file(file.path())};

  CHECK(symbols[0].name == "value1");
  CHECK(symbols[0].kind == clang::clangd::SymbolKind::Variable);
  CHECK(symbols[1].name == "value2");
  CHECK(symbols[1].kind == clang::clangd::SymbolKind::Variable);
}
}  // namespace cppcia