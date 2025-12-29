#ifndef RMS_FORCEFIELD_HPP
#define RMS_FORCEFIELD_HPP

#include "parsers.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace rms {

[[nodiscard]] std::vector<int> build_atom_residue_map(const Parm7Topology &topo);

[[nodiscard]] std::optional<std::size_t> lj_pair_index(const Parm7Topology &topo, int type_i, int type_j);

[[nodiscard]] std::optional<std::pair<double, double>> lj_pair_coeffs(const Parm7Topology &topo, int type_i,
  int type_j);

} // namespace rms

#endif // RMS_FORCEFIELD_HPP
