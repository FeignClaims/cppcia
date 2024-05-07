#include "cppcia/cppcia_main.hpp"

#include "cppcia/dot.hpp"
#include "cppcia/extractor.hpp"
#include "cppcia/graph_util.hpp"
#include "cppcia/reference.hpp"
#include "cppcia/referencer.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gsl/gsl>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <clang/Tooling/Execution.h>
#include <clangd/Protocol.h>
#include <clangd/index/Index.h>
#include <clangd/index/MemIndex.h>
#include <clangd/index/Serialization.h>
#include <clangd/support/Logger.h>
#include <ctre.hpp>
#include <fmt/core.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>

namespace cppcia {
namespace {
  using llvm::cl::cat;
  using llvm::cl::CommaSeparated;
  using llvm::cl::desc;
  using llvm::cl::list;
  using llvm::cl::opt;
  using llvm::cl::OptionCategory;
  using llvm::cl::Positional;
  using llvm::cl::Required;
  using llvm::cl::ValueDisallowed;

  using Path = std::string;

  [[nodiscard]] auto existing_absolute(std::filesystem::path const& path) -> Path {
    if (!std::filesystem::exists(path)) {
      throw std::invalid_argument{fmt::format("Path {} dosen't exist!", path.string())};
    }
    return std::filesystem::absolute(path).string();
  }
  [[nodiscard]] auto existing_absolute(Path const& path) -> Path {
    return existing_absolute(std::filesystem::path{path});
  }
  [[nodiscard]] auto absolute(std::filesystem::path const& path) -> Path {
    return std::filesystem::absolute(path).string();
  }
  [[nodiscard]] auto absolute(Path const& path) -> Path {
    return ::cppcia::absolute(std::filesystem::path{path});
  }

  // NOLINTBEGIN(*non-const-global*, cert-err58-cpp)
  namespace option {
    OptionCategory index{"cppcia index options"};
    opt<Path> index_file{Positional, Required, cat{index}, desc{"<index_file>"}};
    opt<Path> compile_commands_dir{Positional, Required, cat{index}, desc{"<compile_commands_dir>"}};
    opt<Path> resource_dir{"resource-dir",
                           cat{index},
                           desc{"Directory for clang resource (i.e. headers). "
                                "If not set, system clang resource directory is obtained"}};
    list<std::string> query_driver_globs{"query-driver",
                                         CommaSeparated,
                                         cat{index},
                                         desc{"Comma separated list of globs for white-listing gcc-compatible "
                                              "drivers that are safe to execute. Drivers matching any of these globs "
                                              "will be used to extract system includes. e.g. "
                                              "/usr/bin/**/clang-*,/path/to/repo/**/g++-*"}};

    OptionCategory input{"cppcia input options"};
    list<Path> file{"file",
                    cat{input},
                    desc{"File queries. "
                         "Specify by <path>. "
                         "e.g. --file src/main.cpp"}};
    list<std::string> location{"location",
                               cat{input},
                               desc{"Locations queries. "
                                    "Specify by <path>:<line>:<column>. "
                                    "Note that <line> and <column> start from 0. "
                                    "e.g. src/main.cpp:3:5"}};
    list<std::string> name{"name",
                           cat{input},
                           desc{"Name queries. "
                                "Name specfied by [namespace::][::]<name> must match exactly. "
                                "e.g. std::array"}};
    list<std::string> name_fuzzy{"name-fuzzy",
                                 cat{input},
                                 desc{"Fuzzy name queries. "
                                      "Name specfied by [namespace::][::]<name> can ignore some letters. "
                                      "e.g. array"}};
    opt<bool> follow_contain_by{
        "follow-contain-by", ValueDisallowed, cat{input}, desc{"Query result following contain-by impacts"}};
    opt<bool> follow_call{"follow-call", ValueDisallowed, cat{input}, desc{"Query result following call impacts"}};
    opt<bool> follow_supertype{
        "follow-supertype", ValueDisallowed, cat{input}, desc{"Query result following subtype impacts"}};
    opt<bool> follow_subtype{
        "follow-subtype", ValueDisallowed, cat{input}, desc{"Query result following subtype impacts"}};

    OptionCategory output{"cppcia output Options"};
    opt<Path> output_file{Positional, Required, cat{output}, desc{"<output_file>"}};
    opt<Path> workspace_root{
        "workspace-root",
        cat{output},
        desc{"All paths in output graph will relative to this path. "
             "If not specified, paths in output graph will be absolute"},
    };
    opt<bool> file_level{"file-level", ValueDisallowed, cat{output}, desc{"Output file level graph"}};

    std::array const categories{&index, &input, &output};
  }  // namespace option
  // NOLINTEND(*non-const-global*, cert-err58-cpp)

  [[nodiscard]] auto parse_location(llvm::StringRef location) -> std::pair<std::string, clang::clangd::Position> {
    using namespace ctre::literals;  // NOLINT(*using-namespace*)
    auto [whole, file, line, character]{R"ctre((.*?):(.*?):(.*?))ctre"_ctre.match(location)};
    if (!whole) {
      throw std::invalid_argument{fmt::format("{} is not a valid location!", location.data())};
    }
    return {file.to_string(), clang::clangd::Position{line.to_number(), character.to_number()}};
  }

  [[nodiscard]] auto reference_call(Referencer& referencer,  // NOLINT(*recursion*)
                                    Reference const& reference) -> Reference_graph {
    switch (reference.kind) {
      using enum SymbolKind;
      case Constructor:
      case Function:
      case Interface:
      case Method:
      case Operator:
        return to_graph(referencer.find_caller_hierarchies(reference), Edge_type::dashed, /*reverse_edge=*/true);

      case Array:
      case Boolean:
      case Class:
      case Constant:
      case Enum:
      case EnumMember:
      case Event:
      case Field:
      case File:
      case Key:
      case Module:
      case Namespace:
      case Null:
      case Number:
      case Object:
      case Package:
      case Property:
      case String:
      case Struct:
      case TypeParameter:
      case Variable:
        return {};
    }
    return {};  // FIXME: unreachable
  }

  [[nodiscard]] auto reference_supertype(Referencer& referencer,  // NOLINT(*recursion*)
                                         Reference const& reference) -> Reference_graph {
    switch (reference.kind) {
      using enum SymbolKind;
      case Class:
      case Enum:
      case Struct:
        return to_graph(referencer.find_supertype_hierarchies(reference), Edge_type::dashed, /*reverse_edge=*/true);

      case Array:
      case Boolean:
      case Constant:
      case Constructor:
      case EnumMember:
      case Event:
      case Field:
      case File:
      case Function:
      case Interface:
      case Key:
      case Method:
      case Module:
      case Namespace:
      case Null:
      case Number:
      case Object:
      case Operator:
      case Package:
      case Property:
      case String:
      case TypeParameter:
      case Variable:
        return {};
    }
    return {};  // FIXME: unreachable
  }

  [[nodiscard]] auto reference_subtype(Referencer& referencer,  // NOLINT(*recursion*)
                                       Reference const& reference) -> Reference_graph {
    switch (reference.kind) {
      using enum SymbolKind;
      case Class:
      case Enum:
      case Struct:
        return to_graph(referencer.find_subtype_hierarchies(reference), Edge_type::dashed, /*reverse_edge=*/false);

      case Array:
      case Boolean:
      case Constant:
      case Constructor:
      case EnumMember:
      case Event:
      case Field:
      case File:
      case Function:
      case Interface:
      case Key:
      case Method:
      case Module:
      case Namespace:
      case Null:
      case Number:
      case Object:
      case Operator:
      case Package:
      case Property:
      case String:
      case TypeParameter:
      case Variable:
        return {};
    }
    return {};  // FIXME: unreachable
  }

  [[nodiscard]] auto reference_on_option(Referencer& referencer, Reference const& reference) -> Reference_graph {
    Reference_graph result{to_graph(referencer.find_references(reference), Edge_type::solid, /*reverse_edge=*/false)};
    if (option::follow_contain_by) {
      auto path{referencer.find_container_path(reference)};
      merge_by(result, to_graph(path, Edge_type::dashed, true));

      if (option::follow_call || option::follow_subtype || option::follow_supertype) {
        visit(path, [&](Reference const& node) {
          if (option::follow_call) {
            merge_by(result, reference_call(referencer, node));
          }
          if (option::follow_supertype) {
            merge_by(result, reference_supertype(referencer, node));
          }
          if (option::follow_subtype) {
            merge_by(result, reference_subtype(referencer, node));
          }
        });
      }
    }
    return result;
  }

  [[nodiscard]] auto impact_file(Referencer& referencer, llvm::StringRef file) -> Reference_graph {
    Reference_graph result{};
    auto root{referencer.query_file(file)};
    merge_by(result, to_graph(root, Edge_type::solid, /*reverse_edge=*/false));
    for (auto& child : root.children) {
      visit(child, [&](Reference const& reference) { merge_by(result, reference_on_option(referencer, reference)); });
    }
    return result;
  }

  [[nodiscard]] auto impact_location(Referencer& referencer,
                                     llvm::StringRef file,
                                     clang::clangd::Position pos) -> Reference_graph {
    Reference_graph result{};

    auto queried{referencer.query_location(file, pos)};
    if (!queried) {
      throw std::invalid_argument{fmt::format("No symbol found in {}:{}:{}", file.data(), pos.line, pos.character)};
    }

    merge_by(result, reference_on_option(referencer, *queried));

    return result;
  }

  [[nodiscard]] auto impact_name(Referencer& referencer, llvm::StringRef name, bool fuzzy) -> Reference_graph {
    Reference_graph result{};

    auto querieds{referencer.query_name(name, fuzzy)};
    for (auto& queried : querieds) {
      merge_by(result, reference_on_option(referencer, queried));
    }

    return result;
  }

  [[nodiscard]] auto build_graph(Referencer& referencer) -> Reference_graph {
    Reference_graph result;

    for (auto const& file : option::file) {
      merge_by(result, impact_file(referencer, existing_absolute(file)));
    }

    for (auto const& location : option::location) {
      auto [file, pos]{parse_location(location)};
      merge_by(result, impact_location(referencer, existing_absolute(file), pos));
    }

    for (auto const& name : option::name) {
      merge_by(result, impact_name(referencer, name, false));
    }

    for (auto const& name : option::name_fuzzy) {
      merge_by(result, impact_name(referencer, name, true));
    }

    return result;
  }

  [[nodiscard]] auto adjust_graph(Referencer& /*referencer*/, Reference_graph graph) -> Reference_graph {
    if (option::file_level) {
      return map(graph, [](Reference const& reference) { return make_file_reference(reference.uri.file()); });
    }
    return graph;
  }
}  // namespace

[[nodiscard]] auto cppcia_main(int argc, gsl::czstring argv[]) noexcept -> int {  // NOLINT(*c-array*)
  llvm::cl::HideUnrelatedOptions({option::categories.data(), option::categories.size()});
  llvm::cl::ParseCommandLineOptions(argc,
                                    argv,
                                    R"overview(Query corresponding impacts of symbols for a whole project.

  To use this tool, you need to first index the project with compile_commands.json:
  
  (you can download a clangd-indexer in https://github.com/clangd/clangd/releases)
  $ clangd-indexer --executor=all-TUs compile_commands.json > clangd.dex

  After that, query impacts of symbols as you like.
  For example, the following is how you query a file impact:

  $ cppcia <index_file> <compile_commands_dir> <output_file> -f <file_to_be_queried>
)overview");

  if (option::follow_call || option::follow_subtype || option::follow_supertype) {
    option::follow_contain_by = true;
  }

  Referencer referencer{make_extractor(existing_absolute(option::index_file),
                                       existing_absolute(option::compile_commands_dir),
                                       option::resource_dir.empty() ? "" : existing_absolute(option::resource_dir),
                                       std::move(option::query_driver_globs))};

  Reference_graph graph{adjust_graph(referencer, build_graph(referencer))};

  std::ofstream ofile{absolute(option::output_file)};
  format_to_in_dot(ofile,
                   graph,
                   Reference_writer{option::workspace_root.empty()
                                        ? std::optional<std::filesystem::path>{std::nullopt}
                                        : std::optional<std::filesystem::path>{absolute(option::workspace_root)}},
                   edge_type_writer);

  return 0;
}
}  // namespace cppcia
