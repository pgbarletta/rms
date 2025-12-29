#include "include/cli.hpp"
#include "include/forcefield.hpp"
#include "include/parsers.hpp"

#include <internal_use_only/config.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <numeric>
#include <string_view>

namespace {
[[nodiscard]] bool has_flag(int argc, char const *const argv[], std::string_view flag) {
  for (int i = 1; i < argc; ++i) {
    if (flag == argv[i]) {
      return true;
    }
  }
  return false;
}
} // namespace

int main(int argc, char const *const argv[]) {
  if (has_flag(argc, argv, "--version")) {
    fmt::println("{} {}", rms::cmake::project_name, rms::cmake::project_version);
    return 0;
  }
  if (has_flag(argc, argv, "--help") || has_flag(argc, argv, "-h")) {
    rms::parse_cli(argc, argv);
    return 0;
  }

  auto const options = rms::parse_cli(argc, argv);
  if (!options) {
    return 1;
  }

  try {
    auto topo = rms::parse_parm7_file(options->parm7_path);

    double const total_mass = std::accumulate(topo.mass.begin(), topo.mass.end(), 0.0);
    double const total_charge = std::accumulate(topo.charge.begin(), topo.charge.end(), 0.0);

    fmt::println("Title: {}", topo.title.empty() ? "<none>" : topo.title);
    if (!topo.version.empty()) {
      fmt::println("Version: {}", topo.version);
    }
    fmt::println("Atoms: {}", topo.pointers.natom);
    fmt::println("Residues: {}", topo.pointers.nres);
    fmt::println("LJ types: {}", topo.pointers.ntypes);
    fmt::println("Bonds: {} (with H: {}, without H: {})", topo.bond_i.size(), topo.pointers.nbonh,
      topo.pointers.nbona);
    fmt::println("Angles: {} (with H: {}, without H: {})", topo.angle_i.size(), topo.pointers.ntheth,
      topo.pointers.ntheta);
    fmt::println("Dihedrals: {} (with H: {}, without H: {})", topo.dihedral_i.size(), topo.pointers.nphih,
      topo.pointers.nphia);
    fmt::println("Excluded pairs: {}", topo.pointers.nnb);
    fmt::println("Extra points: {}", topo.pointers.numextra);
    fmt::println("Total mass (amu): {:.6f}", total_mass);
    fmt::println("Total charge (e): {:.6f}", total_charge);

    if (topo.box_dimensions) {
      auto const &box = *topo.box_dimensions;
      fmt::println("Box: IFBOX={}, angle={:.6f}, a={:.6f}, b={:.6f}, c={:.6f}", topo.pointers.ifbox, box[0], box[1],
        box[2], box[3]);
    } else {
      fmt::println("Box: IFBOX={}, none", topo.pointers.ifbox);
    }

    if (topo.solvent_pointers) {
      auto const &sol = *topo.solvent_pointers;
      fmt::println("Solvent pointers: IPTRES={}, NSPM={}, NSPSOL={}", sol[0], sol[1], sol[2]);
    }

    if (!topo.radius_set.empty()) {
      fmt::println("Radii set: {}", topo.radius_set);
    }

    if (options->sample_count > 0) {
      std::size_t const sample_count = std::min<std::size_t>(options->sample_count, topo.atom_name.size());
      auto const atom_to_res = rms::build_atom_residue_map(topo);

      fmt::println("Sample atoms (first {}):", sample_count);
      for (std::size_t atom = 0; atom < sample_count; ++atom) {
        int const res = atom_to_res[atom];
        std::string_view res_label = "<none>";
        int res_index = 0;
        if (res >= 0 && static_cast<std::size_t>(res) < topo.residue_label.size()) {
          res_label = topo.residue_label[static_cast<std::size_t>(res)];
          res_index = res + 1;
        }

        auto const name = topo.atom_name[atom];
        auto const amber_type = topo.amber_atom_type[atom];
        int const lj_type = topo.atom_type_index[atom];

        auto const lj_idx = rms::lj_pair_index(topo, lj_type, lj_type);
        auto const lj_coeffs = rms::lj_pair_coeffs(topo, lj_type, lj_type);

        fmt::println("Atom {:>6} {:<4} res {:<4} {}", atom + 1, name, res_label, res_index);
        fmt::println("  Z={} mass={:.6f} charge={:.6f} amber_type={}", topo.atomic_number[atom], topo.mass[atom],
          topo.charge[atom], amber_type);
        if (lj_type >= 0) {
          if (lj_idx && lj_coeffs) {
            fmt::println("  LJ type={} index={} A={:.6f} B={:.6f}", lj_type + 1, *lj_idx + 1, lj_coeffs->first,
              lj_coeffs->second);
          } else {
            fmt::println("  LJ type={} index=NA", lj_type + 1);
          }
        } else {
          fmt::println("  LJ type=NA");
        }
      }
    }
  } catch (const std::exception &e) {
    fmt::println(stderr, "Error: {}", e.what());
    return 1;
  }

  return 0;
}
