#include <cassert>
#include <memory>

#include <clang/AST/ASTImporter.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Tooling/Tooling.h>

template <typename Node, typename Matcher>
[[nodiscard]] auto getFirstDecl(Matcher matcher, std::unique_ptr<clang::ASTUnit> const& Unit) -> Node* {
  auto bind         = matcher.bind("bind");  // Bind the to-be-matched node to a string key.
  auto match_result = clang::ast_matchers::match(bind, Unit->getASTContext());
  // We should have at least one match.
  assert(match_result.size() >= 1);
  // Get the first matched and bound node.
  Node* result = const_cast<Node*>(match_result[0].template getNodeAs<Node>("bind"));
  assert(result);
  return result;
}

auto main() -> int {
  auto to_unit{clang::tooling::buildASTFromCode("", "to.cc")};
  auto from_unit{clang::tooling::buildASTFromCode(
      R"(
      class MyClass {
        int m1;
        int m2;
      };
      )",
      "from.cc")};
  auto matcher{clang::ast_matchers::cxxRecordDecl(clang::ast_matchers::hasName("MyClass"))};
  auto from{getFirstDecl<clang::CXXRecordDecl>(matcher, from_unit)};

  clang::ASTImporter importer{to_unit->getASTContext(),
                              to_unit->getFileManager(),
                              from_unit->getASTContext(),
                              from_unit->getFileManager(),
                              /*MinimalImport=*/true};
  llvm::Expected<clang::Decl*> imported_or_error = importer.Import(from);
  if (!imported_or_error) {
    llvm::Error error{imported_or_error.takeError()};
    llvm::errs() << "ERROR: " << error << "\n";
    consumeError(std::move(error));
    return 1;
  }
  clang::Decl* imported{*imported_or_error};
  imported->getTranslationUnitDecl()->dump();

  if (llvm::Error error{importer.ImportDefinition(from)}; error) {
    llvm::errs() << "ERROR: " << error << "\n";
    consumeError(std::move(error));
    return 1;
  }
  llvm::errs() << "Imported definition.\n";
  imported->getTranslationUnitDecl()->dump();

  return 0;
}