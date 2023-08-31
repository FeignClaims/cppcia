#include <memory>

#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

struct DumpASTAction : public clang::ASTFrontendAction {
 public:
  [[nodiscard]] auto CreateASTConsumer(clang::CompilerInstance& /*compiler_instance*/, llvm::StringRef /*in_file*/)
      -> std::unique_ptr<clang::ASTConsumer> override {
    return clang::CreateASTDumper(nullptr, "", true, true, false, true, clang::ASTDumpOutputFormat::ADOF_Default);
  }
};

namespace {
// NOLINTBEGIN(*global*, *err58*)
llvm::cl::OptionCategory tool_category("metareflect options");
llvm::cl::extrahelp common_help{clang::tooling::CommonOptionsParser::HelpMessage};
// NOLINTEND(*global*, *err58*)
}  // namespace

auto main(int argc, char const* argv[]) -> int {
  llvm::Expected<clang::tooling::CommonOptionsParser> expected_parser{
      clang::tooling::CommonOptionsParser::create(argc, argv, tool_category)};
  if (!expected_parser) {
    llvm::errs() << expected_parser.takeError();
    return 1;
  }
  clang::tooling::CommonOptionsParser& options_parser{*expected_parser};
  clang::tooling::ClangTool tool{options_parser.getCompilations(), options_parser.getSourcePathList()};

  return tool.run(clang::tooling::newFrontendActionFactory<DumpASTAction>().get());
}