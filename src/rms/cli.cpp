#include "CLI/CLI.hpp"
#include "include/cli.hpp"

#include <fmt/format.h>

namespace rms {

std::optional<CliOptions> parse_cli(int argc, char const *const argv[]) {
  CLI::App app{"rms: parse Amber parm7/prmtop topologies and print a summary"};

  CliOptions options{};
  app.add_option("parm7", options.parm7_path, "Path to Amber parm7/prmtop topology file")
    ->required();
  app.add_option("--sample", options.sample_count,
    "Number of atoms to sample for force field details (0 to disable)")
    ->default_val(5);

  try {
    app.parse(argc, argv);
  } catch (const CLI::CallForHelp &) {
    fmt::print(stderr, "{}", app.help());
    return std::nullopt;
  } catch (const CLI::ParseError &e) {
    fmt::println(stderr, "Bad input: {}\n{}", e.what(), app.help());
    return std::nullopt;
  }

  return options;
}

} // namespace rms
