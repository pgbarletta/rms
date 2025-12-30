#ifndef RMS_CLI_HPP
#define RMS_CLI_HPP

#include <cstddef>
#include <filesystem>
#include <optional>

namespace rms {

struct CliOptions {
  std::filesystem::path parm7_path;
  std::size_t sample_count = 5;
};

std::optional<CliOptions> parse_cli(int argc, char const *const argv[]);

} // namespace rms

#endif // RMS_CLI_HPP
