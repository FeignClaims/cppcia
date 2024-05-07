#include "cppcia/referencer.hpp"

#include "cppcia/reference.hpp"
#include "cppcia/test/annotations.hpp"
#include "cppcia/test/referencer.hpp"

#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

namespace cppcia {
TEST_CASE("query_location", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          namespace a::b {
                                          struct Foo {
                                           public:
                                            [[nodiscard]] static constexpr auto ^bar(int lhs, int rhs) -> int {
                                              return lhs + rhs;
                                            }
                                          };
                                          }  // namespace a::b

                                          int main() {
                                            a::b::Foo::bar(3, 5);
                                          }
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> reference{referencer.query_location(file.path(), file.annotations().point())};
  CHECK(reference.has_value());
  CHECK(reference->name_range.contains(file.annotations().point()));
  CHECK(reference->uri.file() == file.path());
  CHECK(reference->name == "bar");
  CHECK(reference->namespace_scopes == "a::b::");
  CHECK(reference->local_scopes == "Foo::");
}

TEST_CASE("query_name", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          namespace a::b {
                                          struct Foo {
                                           public:
                                            [[nodiscard]] static constexpr auto bar(int lhs, int rhs) -> int {
                                              return lhs + rhs;
                                            }
                                          };

                                          void foobar() {}

                                          inline namespace cpo {
                                            inline constexpr int Foo{};
                                          }  // namespace cpo
                                          }  // namespace a::b

                                          int main() {
                                            a::b::Foo::bar(3, 5);
                                          }
                                        )cpp"}};
  referencer.update_file(file.path(), file.annotations().code());

  std::vector<Reference> vertices{referencer.query_name("Foo", true)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_references", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          [[nodiscard]] constexpr auto $main^add(int lhs, int rhs) -> int {
                                            return lhs + rhs;
                                          }

                                          int main() {
                                            $1^add(4, 5);
                                            $2^add(6, 7);
                                          }
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> reference{referencer.query_location(file.path(), file.annotations().point("main"))};
  REQUIRE(reference.has_value());

  auto [root, children]{referencer.find_references(*reference)};
  CHECK(root.name_range.contains(file.annotations().point("main")));
  CHECK(root.uri.file() == file.path());
  CHECK(root.namespace_scopes.empty());
  CHECK(root.local_scopes.empty());
  CHECK(root.name == "add");

  CHECK(children[0].reference.name_range.contains(file.annotations().point("1")));
  CHECK(children[0].reference.uri.file() == file.path());
  CHECK(children[0].reference.namespace_scopes.empty());
  CHECK(children[0].reference.local_scopes.empty());
  CHECK(children[0].reference.name == "add");
  CHECK(children[0].children.empty());

  CHECK(children[1].reference.name_range.contains(file.annotations().point("2")));
  CHECK(children[1].reference.uri.file() == file.path());
  CHECK(children[1].reference.namespace_scopes.empty());
  CHECK(children[1].reference.local_scopes.empty());
  CHECK(children[1].reference.name == "add");
  CHECK(children[1].children.empty());
}

TEST_CASE("find_direct_callers", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          void f1() {}
                                          void f2() {
                                            f1();
                                          }
                                          void f3() {
                                            f2();
                                          }
                                          void g3() {
                                            ^f2();
                                          }
                                        )cpp"}};
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> callee{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(callee.has_value());

  std::vector<Reference> callers{referencer.find_direct_callers(*callee)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_caller_hierarchies", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          void f1() {}
                                          void f2() {
                                            f1();
                                          }
                                          void f3() {
                                            f2();
                                          }
                                          void g3() {
                                            ^f2();
                                          }
                                        )cpp"}};
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> callee{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(callee.has_value());

  Reference_tree callers{referencer.find_caller_hierarchies(*callee)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_direct_supertypes", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          class A1 {};
                                          class A2 {};

                                          class B1 : public A1 {};
                                          class B2 : public A2 {};

                                          class ^C : public B1, public B2 {};

                                          class D1 : public C {};
                                          class D2 : public C {};

                                          class E1 : public D1 {};
                                          class E2 : public D2 {};
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> type{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(type.has_value());

  std::vector<Reference> supertypes{referencer.find_direct_supertypes(*type)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_supertype_hierarchies", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          class A1 {};
                                          class A2 {};

                                          class B1 : public A1 {};
                                          class B2 : public A2 {};

                                          class ^C : public B1, public B2 {};

                                          class D1 : public C {};
                                          class D2 : public C {};

                                          class E1 : public D1 {};
                                          class E2 : public D2 {};
                                          }
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> type{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(type.has_value());

  Reference_tree supertypes{referencer.find_supertype_hierarchies(*type)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_direct_subtypes", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          class A1 {};
                                          class A2 {};

                                          class B1 : public A1 {};
                                          class B2 : public A2 {};

                                          class ^C : public B1, public B2 {};

                                          class D1 : public C {};
                                          class D2 : public C {};

                                          class E1 : public D1 {};
                                          class E2 : public D2 {};
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> type{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(type.has_value());

  std::vector<Reference> subtypes{referencer.find_direct_subtypes(*type)};
  // FIXME: Can't test index-based operations
}

TEST_CASE("find_subtype_hierarchies", "[referencer]") {
  Referencer referencer{make_referencer_for_test()};
  // clang-format off
  Mock_file file{"foo.cpp", Annotations{R"cpp(
                                          class A1 {};
                                          class A2 {};

                                          class B1 : public A1 {};
                                          class B2 : public A2 {};

                                          class ^C : public B1, public B2 {};

                                          class D1 : public C {};
                                          class D2 : public C {};

                                          class E1 : public D1 {};
                                          class E2 : public D2 {};
                                        )cpp"}};
  // clang-format on
  referencer.update_file(file.path(), file.annotations().code());

  std::optional<Reference> type{referencer.query_location(file.path(), file.annotations().point(""))};
  REQUIRE(type.has_value());

  Reference_tree subtypes{referencer.find_subtype_hierarchies(*type)};
  // FIXME: Can't test index-based operations
}
}  // namespace cppcia