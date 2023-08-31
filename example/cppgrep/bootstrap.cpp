#include <cassert>
#include <cstdlib>
#include <fstream>
#include <gsl/gsl>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include <clang-c/Index.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/CommandLine.h>

namespace {
// NOLINTBEGIN(*global*, *cert-err58*)
llvm::cl::OptionCategory cppgrep_category{"Cppgrep Options"};

llvm::cl::opt<std::string> pattern_option{llvm::cl::Positional, llvm::cl::Required, llvm::cl::desc("<pattern>")};

llvm::cl::list<std::string> files_option{
    llvm::cl::Positional, llvm::cl::OneOrMore, llvm::cl::desc("<file> [files...]")};

llvm::cl::opt<bool> case_insensitive_option{
    "i", llvm::cl::desc("Make the search case-insensitive"), llvm::cl::cat(cppgrep_category)};

llvm::cl::opt<bool> function_option{"function", llvm::cl::desc("Filter by functions"), llvm::cl::cat(cppgrep_category)};
llvm::cl::alias function_short_option{"f", llvm::cl::desc("Alias for -function"), llvm::cl::aliasopt(function_option)};

llvm::cl::opt<bool> variable_option{"variable", llvm::cl::desc("Filter by variables"), llvm::cl::cat(cppgrep_category)};
llvm::cl::alias variable_short_option{"v", llvm::cl::desc("Alias for -variable"), llvm::cl::aliasopt(variable_option)};

llvm::cl::opt<bool> record_option{
    "record", llvm::cl::desc("Filter by records (class/struct)"), llvm::cl::cat(cppgrep_category)};
llvm::cl::alias record_short_option{"r", llvm::cl::desc("Alias for -record"), llvm::cl::aliasopt(record_option)};

llvm::cl::opt<bool> parameter_option{
    "parameter", llvm::cl::desc("Filter by function parameter"), llvm::cl::cat(cppgrep_category)};
llvm::cl::alias parameter_short_option{
    "p", llvm::cl::desc("Alias for -parameter"), llvm::cl::aliasopt(parameter_option)};

llvm::cl::opt<bool> member_option{"member", llvm::cl::desc("Filter by members"), llvm::cl::cat(cppgrep_category)};
llvm::cl::alias member_short_option{"m", llvm::cl::desc("Alias for -member"), llvm::cl::aliasopt(member_option)};
// NOLINTEND(*global*, *cert-err58*)
}  // namespace

class [[nodiscard]] Filter {
 public:
  using Predicate = std::function<bool(CXCursor)>;

  explicit Filter(Predicate&& pattern) : pattern_{std::move(pattern)} {}

  void add(Predicate&& predicate) {
    predicates_.emplace_back(std::move(predicate));
  }

  [[nodiscard]] auto matches(CXCursor cursor) const noexcept -> bool {
    if (!pattern_(cursor)) {
      return false;
    }
    if (predicates_.empty()) {
      return true;
    }

    return std::any_of(predicates_.begin(), predicates_.end(), [cursor](auto& predicate) { return predicate(cursor); });
  }

 private:
  Predicate pattern_;
  std::vector<Predicate> predicates_;
};

struct [[nodiscard]] Data {
 public:
  using Lines = std::vector<std::string>;

  Filter filter;
  Lines lines{};
};

[[nodiscard]] auto to_string(CXString cxString) -> std::string {
  std::string string{clang_getCString(cxString)};
  clang_disposeString(cxString);
  return string;
}

void display_match(CXSourceLocation location, CXCursor cursor, Data::Lines const& lines) {
  CXFile file{nullptr};
  unsigned lineNumber{};
  unsigned columnNumber{};
  clang_getSpellingLocation(location, &file, &lineNumber, &columnNumber, nullptr);

  Expects(lineNumber - 1 < lines.size());

  if (files_option.size() > 1) {
    std::cout << to_string(clang_getFileName(file)) << ':';
  }

  std::cout << "\033[1m" << lineNumber << ':' << columnNumber << "\033[0m: ";

  auto const& line{lines[lineNumber - 1]};
  for (unsigned column{1}; column <= line.length(); ++column) {
    if (column == columnNumber) {
      auto const spelling{to_string(clang_getCursorSpelling(cursor))};
      std::cout << "\033[1;91m" << spelling << "\033[0m";
      column += spelling.length() - 1;
    } else {
      std::cout << line[column - 1];
    }
  }

  std::cout << '\n';
}

[[nodiscard]] auto grep(CXCursor cursor, CXCursor /*unused*/, CXClientData client_data) -> CXChildVisitResult {
  CXSourceLocation const location{clang_getCursorLocation(cursor)};
  if (clang_Location_isInSystemHeader(location)) {
    return CXChildVisit_Continue;
  }

  Data const* data{static_cast<Data*>(client_data)};
  if (data->filter.matches(cursor)) {
    display_match(location, cursor, data->lines);
  }

  return CXChildVisit_Recurse;
}

[[nodiscard]] auto parse(CXIndex index, std::string const& filename) -> CXTranslationUnit {
  CXTranslationUnit traslation_unit{clang_parseTranslationUnit(index,
                                                               /*source_filename=*/filename.c_str(),
                                                               /*command_line_args=*/nullptr,
                                                               /*num_command_line_args=*/0,
                                                               /*unsaved_files=*/nullptr,
                                                               /*num_unsaved_files=*/0,
                                                               /*options=*/0)};
  if (!traslation_unit) {
    std::cerr << "Error parsing file: '" << filename << "'\n";
  }

  return traslation_unit;
}

auto make_pattern_predicate() -> Filter::Predicate {
  std::regex_constants::syntax_option_type regex_options{std::regex::ECMAScript | std::regex::optimize};
  if (case_insensitive_option) {
    regex_options |= std::regex::icase;
  }

  std::regex const pattern(pattern_option, regex_options);

  return [pattern](CXCursor cursor) {
    std::string const spelling{to_string(clang_getCursorSpelling(cursor))};
    return std::regex_search(spelling, pattern);
  };
}

[[nodiscard]] auto make_filter() -> Filter {
  Filter filter(make_pattern_predicate());

  if (function_option) {
    filter.add([](CXCursor cursor) {
      CXCursorKind const kind{clang_getCursorKind(cursor)};
      if (member_option) {
        return kind == CXCursor_CXXMethod;
      }
      return kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod;
    });
  }

  if (variable_option) {
    filter.add([](CXCursor cursor) {
      CXCursorKind const kind{clang_getCursorKind(cursor)};
      if (member_option) {
        return kind == CXCursor_FieldDecl;
      }
      return kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl;
    });
  }

  if (parameter_option) {
    filter.add([](CXCursor cursor) { return clang_getCursorKind(cursor) == CXCursor_ParmDecl; });
  }

  if (member_option && !variable_option && !parameter_option) {
    filter.add([](CXCursor cursor) {
      CXCursorKind const kind{clang_getCursorKind(cursor)};
      return kind == CXCursor_FieldDecl || kind == CXCursor_CXXMethod;
    });
  }

  if (record_option) {
    filter.add([](CXCursor cursor) {
      CXCursorKind const kind{clang_getCursorKind(cursor)};
      return kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl;
    });
  }

  return filter;
}

[[nodiscard]] auto read_lines(std::string const& filename) -> Data::Lines {
  Data::Lines lines;

  std::ifstream stream{filename};
  std::string line;
  while (std::getline(stream, line)) {
    lines.emplace_back(line);
  }

  return lines;
}

auto main(int argc, char const* argv[]) -> int {
  llvm::cl::HideUnrelatedOptions(cppgrep_category);
  llvm::cl::ParseCommandLineOptions(argc, argv);

  Data data{make_filter()};

  CXIndex index{clang_createIndex(/*excludeDeclarationsFromPCH=*/true,
                                  /*displayDiagnostics=*/true)};
  for (auto const& filename : files_option) {
    data.lines = read_lines(filename);

    CXTranslationUnit translation_unit{parse(index, filename)};
    if (!translation_unit) {
      break;
    }

    auto cursor{clang_getTranslationUnitCursor(translation_unit)};
    clang_visitChildren(cursor, grep, &data);

    clang_disposeTranslationUnit(translation_unit);
  }
}