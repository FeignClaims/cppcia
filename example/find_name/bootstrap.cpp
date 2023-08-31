#include <gsl/gsl>
#include <memory>

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

class [[nodiscard]] FindNamedClassVisitor : public clang::RecursiveASTVisitor<FindNamedClassVisitor> {
 public:
  explicit FindNamedClassVisitor(gsl::strict_not_null<clang::ASTContext*> context) : context_{context} {}

  [[nodiscard]] auto VisitCXXRecordDecl(clang::CXXRecordDecl* declaration) -> bool {
    if (declaration->getQualifiedNameAsString() == "n::m::C") {
      clang::FullSourceLoc full_location{context_->getFullLoc(declaration->getBeginLoc())};
      if (full_location.isValid()) {
        llvm::outs() << "Found declaration at " << full_location.getSpellingLineNumber() << ':'
                     << full_location.getSpellingColumnNumber() << '\n';
      }
    }
    return true;
  }

 private:
  gsl::strict_not_null<clang::ASTContext*> context_;
};

class [[nodiscard]] FindNamedClassConsumer : public clang::ASTConsumer {
 public:
  explicit FindNamedClassConsumer(gsl::strict_not_null<clang::ASTContext*> context) : visitor_{context} {}

  void HandleTranslationUnit(clang::ASTContext& context) override {
    visitor_.TraverseDecl(context.getTranslationUnitDecl());
  }

 private:
  FindNamedClassVisitor visitor_;
};

class [[nodiscard]] FindNamedClassAction : public clang::ASTFrontendAction {
 public:
  [[nodiscard]] auto CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef /*in_file*/)
      -> std::unique_ptr<clang::ASTConsumer> override {
    return std::make_unique<FindNamedClassConsumer>(gsl::strict_not_null{&compiler.getASTContext()});
  }
};

auto main(int argc, char const* argv[]) -> int {
  if (argc > 1) {
    clang::tooling::runToolOnCode(std::make_unique<FindNamedClassAction>(), argv[1]);
  }
}