#ifndef RMS_PARSERS_HPP
#define RMS_PARSERS_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rms {

constexpr double kAmberChargeScale = 18.2223;
constexpr int kParm7PointerCount = 31;

struct Parm7Pointers {
  int natom = 0;
  int ntypes = 0;
  int nbonh = 0;
  int mbona = 0;
  int ntheth = 0;
  int mtheta = 0;
  int nphih = 0;
  int mphia = 0;
  int nhparm = 0;
  int nparm = 0;
  int nnb = 0;
  int nres = 0;
  int nbona = 0;
  int ntheta = 0;
  int nphia = 0;
  int numbnd = 0;
  int numang = 0;
  int nptra = 0;
  int natyp = 0;
  int nphb = 0;
  int ifpert = 0;
  int nbper = 0;
  int ngper = 0;
  int ndper = 0;
  int mbper = 0;
  int mgper = 0;
  int mdper = 0;
  int ifbox = 0;
  int nmxrs = 0;
  int ifcap = 0;
  int numextra = 0;
  std::optional<int> ncopy;
};

struct Parm7Topology {
  std::string version;
  std::string title;
  Parm7Pointers pointers;

  std::vector<std::string> atom_name;
  std::vector<double> charge;
  std::vector<int> atomic_number;
  std::vector<double> mass;
  std::vector<int> atom_type_index;
  std::vector<int> number_excluded_atoms;
  std::vector<int> excluded_atoms_list;
  std::vector<int> nonbonded_parm_index;
  std::vector<std::string> residue_label;
  std::vector<int> residue_pointer;

  std::vector<double> bond_force_constant;
  std::vector<double> bond_equil_value;
  std::vector<double> angle_force_constant;
  std::vector<double> angle_equil_value;
  std::vector<double> dihedral_force_constant;
  std::vector<double> dihedral_periodicity;
  std::vector<double> dihedral_phase;
  std::vector<double> scee_scale_factor;
  std::vector<double> scnb_scale_factor;
  std::vector<double> solty;
  std::vector<double> lennard_jones_acoeff;
  std::vector<double> lennard_jones_bcoeff;

  std::vector<int> bond_i;
  std::vector<int> bond_j;
  std::vector<int> bond_type;
  std::vector<int> angle_i;
  std::vector<int> angle_j;
  std::vector<int> angle_k;
  std::vector<int> angle_type;
  std::vector<int> dihedral_i;
  std::vector<int> dihedral_j;
  std::vector<int> dihedral_k;
  std::vector<int> dihedral_l;
  std::vector<int> dihedral_type;
  std::vector<std::uint8_t> dihedral_flags;

  std::vector<double> hbond_acoeff;
  std::vector<double> hbond_bcoeff;
  std::optional<double> hbond_cut;

  std::vector<std::string> amber_atom_type;
  std::vector<std::string> tree_chain_classification;
  std::vector<int> join_array;
  std::vector<int> irotat;

  std::optional<std::array<int, 3>> solvent_pointers;
  std::vector<int> atoms_per_molecule;
  std::optional<std::array<double, 4>> box_dimensions;

  std::string radius_set;
  std::vector<double> radii;
  std::vector<double> screen;
  std::optional<int> ipol;
};

[[nodiscard]] Parm7Topology parse_parm7_file(const std::filesystem::path &path);

} // namespace rms

#endif // RMS_PARSERS_HPP
