#include "include/parsers.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace {
[[nodiscard]] std::filesystem::path parse_path(int argc, char const *const argv[]) {
  if (argc < 2) {
    return {};
  }
  return std::filesystem::path(argv[1]);
}

[[nodiscard]] int parse_iterations(int argc, char const *const argv[]) {
  if (argc < 3) {
    return 5;
  }
  try {
    return std::max(1, std::stoi(argv[2]));
  } catch (...) {
    return 5;
  }
}
} // namespace

int main(int argc, char const *const argv[]) {
  auto const path = parse_path(argc, argv);
  if (path.empty()) {
    fmt::println(stderr, "Usage: rms_parm7_bench <parm7_path> [iterations]");
    return 1;
  }

  auto const iterations = parse_iterations(argc, argv);
  std::uintmax_t const bytes = std::filesystem::file_size(path);

  std::size_t checksum = 0;
  auto const start = std::chrono::steady_clock::now();
  for (int iter = 0; iter < iterations; ++iter) {
    auto topo = rms::parse_parm7_file(path);
    checksum += topo.atom_name.size();
    checksum += topo.bond_i.size();
  }
  auto const end = std::chrono::steady_clock::now();

  std::chrono::duration<double> const elapsed = end - start;
  double const total_bytes = static_cast<double>(bytes) * static_cast<double>(iterations);
  double const gb_per_s = total_bytes / elapsed.count() / 1.0e9;

  fmt::println("parm7 bytes: {}", bytes);
  fmt::println("iterations: {}", iterations);
  fmt::println("elapsed_s: {:.6f}", elapsed.count());
  fmt::println("throughput_GBps: {:.6f}", gb_per_s);
  fmt::println("checksum: {}", checksum);

  return 0;
}
