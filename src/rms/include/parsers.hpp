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
  // NATOM: total number of atoms.
  std::uint16_t natom = 0;
  // NTYPES: total number of distinct atom types (LJ types).
  std::uint16_t ntypes = 0;
  // NBONH: number of bonds containing hydrogen.
  std::uint16_t nbonh = 0;
  // MBONA: number of bonds not containing hydrogen.
  std::uint16_t mbona = 0;
  // NTHETH: number of angles containing hydrogen.
  std::uint16_t ntheth = 0;
  // MTHETA: number of angles not containing hydrogen.
  std::uint16_t mtheta = 0;
  // NPHIH: number of dihedrals containing hydrogen.
  std::uint16_t nphih = 0;
  // MPHIA: number of dihedrals not containing hydrogen.
  std::uint16_t mphia = 0;
  // NHPARM: currently not used.
  std::uint16_t nhparm = 0;
  // NPARM: currently not used.
  std::uint16_t nparm = 0;
  // NEXT/NNB: total number of excluded atoms.
  std::uint16_t nnb = 0;
  // NRES: number of residues.
  std::uint16_t nres = 0;
  // NBONA: MBONA plus constraint bonds.
  std::uint16_t nbona = 0;
  // NTHETA: MTHETA plus constraint angles.
  std::uint16_t ntheta = 0;
  // NPHIA: MPHIA plus constraint dihedrals.
  std::uint16_t nphia = 0;
  // NUMBND: number of unique bond types.
  std::uint16_t numbnd = 0;
  // NUMANG: number of unique angle types.
  std::uint16_t numang = 0;
  // NPTRA: number of unique dihedral types.
  std::uint16_t nptra = 0;
  // NATYP: number of atom types in parameter file (SOLTY count).
  std::uint16_t natyp = 0;
  // NPHB: number of distinct 10-12 hydrogen bond pair types.
  std::uint16_t nphb = 0;
  // IFPERT: perturbation flag (1 means perturbation info present).
  std::uint16_t ifpert = 0;
  // NBPER: number of bonds to be perturbed.
  std::uint16_t nbper = 0;
  // NGPER: number of angles to be perturbed.
  std::uint16_t ngper = 0;
  // NDPER: number of dihedrals to be perturbed.
  std::uint16_t ndper = 0;
  // MBPER: number of bonds with atoms entirely in perturbed group.
  std::uint16_t mbper = 0;
  // MGPER: number of angles with atoms entirely in perturbed group.
  std::uint16_t mgper = 0;
  // MDPER: number of dihedrals with atoms entirely in perturbed group.
  std::uint16_t mdper = 0;
  // IFBOX: periodic box flag (0 none, 1 orthorhombic, 2 truncated octahedron, 3 triclinic).
  std::uint16_t ifbox = 0;
  // NMXRS: number of atoms in the largest residue.
  std::uint16_t nmxrs = 0;
  // IFCAP: CAP option flag.
  std::uint16_t ifcap = 0;
  // NUMEXTRA: number of extra points (virtual sites).
  std::uint16_t numextra = 0;
  // NCOPY: number of copies for advanced simulations (optional).
  std::optional<std::uint16_t> ncopy;
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
