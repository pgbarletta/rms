#include "include/forcefield.hpp"
#include "include/parsers.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace rms {

std::vector<int> build_atom_residue_map(const Parm7Topology &topo) {
  std::vector<int> atom_to_res(static_cast<std::size_t>(topo.pointers.natom), -1);
  if (topo.residue_pointer.empty()) {
    return atom_to_res;
  }

  int const nres = topo.pointers.nres;
  int const natom = topo.pointers.natom;

  for (int res = 0; res < nres; ++res) {
    int start = topo.residue_pointer[static_cast<std::size_t>(res)];
    int end = (res + 1 < nres) ? topo.residue_pointer[static_cast<std::size_t>(res) + 1U] : natom;

    start = std::clamp(start, 0, natom);
    end = std::clamp(end, start, natom);

    for (int atom = start; atom < end; ++atom) {
      atom_to_res[static_cast<std::size_t>(atom)] = res;
    }
  }

  return atom_to_res;
}

std::optional<std::size_t> lj_pair_index(const Parm7Topology &topo, int type_i, int type_j) {
  if (type_i < 0 || type_j < 0) {
    return std::nullopt;
  }
  int const ntypes = topo.pointers.ntypes;
  if (type_i >= ntypes || type_j >= ntypes) {
    return std::nullopt;
  }

  std::size_t const idx = static_cast<std::size_t>(type_i) * static_cast<std::size_t>(ntypes) +
                          static_cast<std::size_t>(type_j);
  if (idx >= topo.nonbonded_parm_index.size()) {
    return std::nullopt;
  }

  int const param_index = topo.nonbonded_parm_index[idx];
  if (param_index < 0) {
    return std::nullopt;
  }

  return static_cast<std::size_t>(param_index);
}

std::optional<std::pair<double, double>> lj_pair_coeffs(const Parm7Topology &topo, int type_i, int type_j) {
  auto const idx = lj_pair_index(topo, type_i, type_j);
  if (!idx) {
    return std::nullopt;
  }
  if (*idx >= topo.lennard_jones_acoeff.size() || *idx >= topo.lennard_jones_bcoeff.size()) {
    return std::nullopt;
  }

  return std::pair<double, double>{topo.lennard_jones_acoeff[*idx], topo.lennard_jones_bcoeff[*idx]};
}

} // namespace rms
