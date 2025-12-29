# Project Report: rms

## Purpose
- Parses Amber `parm7/prmtop` topology files, validates sections, and prints a system summary.
- Optionally prints force-field details for a small sample of atoms (default: first 5) with LJ self coefficients.
- Provides a reproducible parser microbenchmark and a small fuzz target.

## Key Data and References
- `daux/binder_wcn.parm7`: Example topology used in tests and benchmarking.
- `daux/parm7.pdf`: Format reference for Amber parm7/prmtop.

## Build Targets
- `rms`: CLI that parses a parm7/prmtop file and prints summary + sample atom details.
- `rms_parm7`: Library target with parser + force-field helpers.
- `rms_parm7_bench`: Microbenchmark for parser throughput.
- `fuzz_tester`: libFuzzer target (generic checksum-style fuzzer).

## Public API Surface

### `src/rms/include/parsers.hpp`

Constants:
- `constexpr double kAmberChargeScale`: Charge scaling factor (18.2223). File charges are divided by this.
- `constexpr int kParm7PointerCount`: Minimum POINTERS entries (31).

Structs:
- `Parm7Pointers`: Holds the POINTERS section values (e.g., NATOM, NTYPES, NBONH, etc.).
  - `ncopy` is optional for extended topologies.
- `Parm7Topology`: Parsed topology, stored as SoA vectors.
  - Atom indices are 0-based.
  - Parameter indices are 0-based.
  - `dihedral_flags` uses bit 0 for suppress-1-4 (negative k) and bit 1 for improper (negative l).

Functions:
- `Parm7Topology parse_parm7_file(const std::filesystem::path &path)`
  - Streaming parser for parm7/prmtop.
  - Parses `%FLAG` sections using `%FORMAT` fixed-width rules.
  - Scales charges by `kAmberChargeScale`.
  - Decodes bonds/angles/dihedrals (3x coordinate index -> atom index; parameter indices 1-based -> 0-based).
  - Validates section sizes against POINTERS and throws on mismatch.

### `src/rms/include/forcefield.hpp`
Functions:
- `std::vector<int> build_atom_residue_map(const Parm7Topology &topo)`
  - Builds atom->residue index mapping from `residue_pointer`.
- `std::optional<std::size_t> lj_pair_index(const Parm7Topology &topo, int type_i, int type_j)`
  - Maps atom LJ types to the parameter index using `nonbonded_parm_index`.
- `std::optional<std::pair<double, double>> lj_pair_coeffs(const Parm7Topology &topo, int type_i, int type_j)`
  - Returns LJ A/B coefficients for a given pair if valid.

### `src/rms/include/utils.hpp`
Helpers:
- `trim_left`, `trim_right`, `trim`: whitespace trimming.
- `to_int`: `std::from_chars` integer parse.
- `to_double`: `std::strtod` parse with `D`->`E` exponent normalization.
- `for_each_token`: whitespace token iterator.

### `src/rms/include/cli.hpp`
- `struct CliOptions { std::filesystem::path parm7_path; std::size_t sample_count = 5; }`.
- `std::optional<CliOptions> parse_cli(int argc, char const *const argv[])`.

## Implementation Details

### `src/rms/parsers.cpp`
Internal helpers:
- `FormatSpec`: `%FORMAT` descriptor (count, type, width).
- `Section` enum + `kSectionMap`: maps `%FLAG` names to parser modes.
- `parse_format_line`, `parse_section_name`: parse `%FORMAT` and `%FLAG`.
- `parse_pointers`: converts POINTERS list to `Parm7Pointers`.
- `reserve_from_pointers`: pre-allocates vectors.
- `append_*` helpers: parse fixed-width sections, with transforms for scaling and 0-basing.
- `decode_bonds`, `decode_angles`, `decode_dihedrals`: convert raw connectivity arrays to atom indices + param indices.
- `require_size`: validates a parsed section length.

Main routine:
- `parse_parm7_file` reads line-by-line, updates section state from `%FLAG`, reads `%FORMAT`, parses data, then validates all sections. It also converts:
  - CHARGE: scaled to elemental charge units.
  - ATOM_TYPE_INDEX, NONBONDED_PARM_INDEX, RESIDUE_POINTER, EXCLUDED_ATOMS_LIST: converted to 0-based indexing.
  - Bond/angle/dihedral pointers: divided by 3 to map to atom indices.

### `src/rms/forcefield.cpp`
- Implements `build_atom_residue_map`, `lj_pair_index`, and `lj_pair_coeffs` with bounds checks.

### `src/rms/cli.cpp`
- CLI11-based parser for `parm7` (required positional) and `--sample` (default 5).

### `src/rms/main.cpp`
- Prints summary fields: title, version, counts, total mass, total charge, box info, solvent pointers, radii set.
- Prints per-atom force-field sample for first `--sample` atoms:
  - Atom id/name, residue label/index
  - Atomic number, mass, charge, amber atom type
  - LJ type and self A/B coefficients

### `src/rms/bench_parm7.cpp`
- Times repeated calls to `parse_parm7_file` for throughput.
- Prints bytes, iterations, elapsed seconds, GB/s, and checksum.

### `scripts/bench_parm7.sh`
- Shell helper to run the benchmark with defaults.
- Uses `set -euo pipefail` and defaults to `daux/binder_wcn.parm7` if no file is passed.

### Tests
- `test/tests.cpp`: Parses `daux/binder_wcn.parm7` and asserts key values, section sizes, residue mapping, and LJ coefficients.
- `test/constexpr_tests.cpp`: Ensures constants are constexpr.
- `test/CMakeLists.txt`: Registers CLI help/version tests and Catch2 suites.

### Fuzz Target
- `fuzz_test/fuzz_tester.cpp`: Example fuzzer that sums bytes; not integrated with the parser.
- `fuzz_test/CMakeLists.txt`: LibFuzzer wiring with a short runtime target.

## Build Notes (CMake)
- Root `CMakeLists.txt`: C++23, target-based configuration, `rms` is the VS startup project.
- `src/rms/CMakeLists.txt`: defines `rms_parm7` library, `rms` CLI, `rms_parm7_bench`.
- `test/CMakeLists.txt`: wires Catch2 tests and uses `RMS_TEST_DATA_DIR` for sample data path.

## Current Limitations / Known Gaps
- `include/rms/sample_library.hpp` declares `factorial(int)` but its implementation is not present in this workspace.
- Fuzz target does not exercise the parser.
