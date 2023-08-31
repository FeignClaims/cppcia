#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

namespace am = clang::ast_matchers;

namespace {
// NOLINTBEGIN(cert-err58-cpp, *global*)
llvm::cl::OptionCategory tool_category{"my-tool options"};
llvm::cl::extrahelp common_help{clang::tooling::CommonOptionsParser::HelpMessage};
llvm::cl::extrahelp more_help{"\nMore help text...\n"};

am::StatementMatcher loop_matcher{
    am::forStmt(am::hasLoopInit(am::declStmt(am::hasSingleDecl(
                    am::varDecl(am::hasInitializer(am::integerLiteral(am::equals(0)))).bind("init_variable")))),
                am::hasCondition(am::binaryOperator(
                    am::hasOperatorName("<"),
                    am::hasLHS(am::ignoringParenImpCasts(
                        am::declRefExpr(am::to(am::varDecl(am::hasType(am::isInteger())).bind("condition_variable"))))),
                    am::hasRHS(am::expr(am::hasType(am::isInteger()))))),
                am::hasIncrement(am::unaryOperator(
                    am::hasOperatorName("++"),
                    am::hasUnaryOperand(am::declRefExpr(
                        am::to(am::varDecl(am::hasType(am::isInteger())).bind("increment_variable")))))))
        .bind("for_loop")};
// NOLINTEND(cert-err58-cpp, *global*)

[[nodiscard]] constexpr auto are_same_variable(clang::ValueDecl const* lhs, clang::ValueDecl const* rhs) -> bool {
  return lhs && rhs && (lhs->getCanonicalDecl() == rhs->getCanonicalDecl());
}

class [[nodiscard]] LoopPrinter : public am::MatchFinder::MatchCallback {
 public:
  void run(am::MatchFinder::MatchResult const& result) override {
    auto context{result.Context};

    if (auto for_statement{result.Nodes.getNodeAs<clang::ForStmt>("for_loop")};
        !for_statement || !context->getSourceManager().isWrittenInMainFile(for_statement->getForLoc())) {
      return;
    }
    auto increment_variable{result.Nodes.getNodeAs<clang::VarDecl>("increment_variable")};
    auto condition_variable{result.Nodes.getNodeAs<clang::VarDecl>("condition_variable")};
    auto init_variable{result.Nodes.getNodeAs<clang::VarDecl>("init_variable")};

    if (!are_same_variable(increment_variable, condition_variable)
        || !are_same_variable(increment_variable, init_variable)) {
      return;
    }
    llvm::outs() << "Potential array-based loop discovered.\n";
  }
};
}  // namespace

auto main(int argc, char const* argv[]) -> int {
  auto expected_parser{clang::tooling::CommonOptionsParser::create(argc, argv, tool_category)};
  if (!expected_parser) {
    llvm::errs() << expected_parser.takeError();
    return 1;
  }
  clang::tooling::CommonOptionsParser& options_parser{expected_parser.get()};
  clang::tooling::ClangTool tool{options_parser.getCompilations(), options_parser.getSourcePathList()};

  LoopPrinter printer{};
  am::MatchFinder finder{};
  finder.addMatcher(loop_matcher, &printer);

  return tool.run(clang::tooling::newFrontendActionFactory<>(&finder).get());
}