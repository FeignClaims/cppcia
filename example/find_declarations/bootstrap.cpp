#include <gsl/gsl>
#include <gsl/pointers>
#include <memory>
#include <string>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <fmt/core.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

namespace {
llvm::cl::extrahelp common_help{clang::tooling::CommonOptionsParser::HelpMessage};

llvm::cl::OptionCategory find_decl_category{"find-decl options"};

[[nodiscard]] auto get_decl_location(clang::SourceManager const& source_manager, clang::SourceLocation location)
    -> std::string {
  return fmt::format("{}:{}:{}",
                     source_manager.getFilename(location).str(),
                     source_manager.getSpellingLineNumber(location),
                     source_manager.getSpellingColumnNumber(location));
}

class [[nodiscard]] DeclVisitor : public clang::RecursiveASTVisitor<DeclVisitor> {
 public:
  explicit DeclVisitor(gsl::strict_not_null<clang::SourceManager*> source_manager) : source_manager_{source_manager} {}

  [[nodiscard]] auto VisitNamedDecl(clang::NamedDecl* named_decl) -> bool {
    llvm::outs() << fmt::format("Found {} at {}\n",
                                named_decl->getQualifiedNameAsString(),
                                get_decl_location(*source_manager_, named_decl->getLocation()));
    return true;
  }

 private:
  gsl::strict_not_null<clang::SourceManager*> source_manager_;
};

class [[nodiscard]] DeclFinder : public clang::ASTConsumer {
 public:
  explicit DeclFinder(gsl::strict_not_null<clang::SourceManager*> source_manager)
      : source_manager_{source_manager}, visitor_{source_manager} {}

  void HandleTranslationUnit(clang::ASTContext& context) final {
    for (auto decls{context.getTranslationUnitDecl()->decls()}; auto& decl : decls) {
      if (auto const& file_id{source_manager_->getFileID(decl->getLocation())};
          file_id == source_manager_->getMainFileID()) {
        visitor_.TraverseDecl(context.getTranslationUnitDecl());
      }
    }
  }

 private:
  gsl::strict_not_null<clang::SourceManager*> source_manager_;
  DeclVisitor visitor_;
};

class [[nodiscard]] DeclFindingAction : public clang::ASTFrontendAction {
 public:
  auto CreateASTConsumer(clang::CompilerInstance& instant, clang::StringRef /*in_file*/)
      -> std::unique_ptr<clang::ASTConsumer> final {
    return std::make_unique<DeclFinder>(gsl::strict_not_null{&instant.getSourceManager()});
  }
};
}  // namespace

auto main(int argc, char const* argv[]) -> int {
  auto expected_parser{clang::tooling::CommonOptionsParser::create(argc, argv, find_decl_category)};
  if (!expected_parser) {
    llvm::errs() << expected_parser.takeError();
    return 1;
  }
  clang::tooling::CommonOptionsParser& options_parser{expected_parser.get()};
  clang::tooling::ClangTool tool{options_parser.getCompilations(), options_parser.getSourcePathList()};

  return tool.run(clang::tooling::newFrontendActionFactory<DeclFindingAction>().get());
}