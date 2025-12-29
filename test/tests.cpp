#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "include/forcefield.hpp"
#include "include/parsers.hpp"

#include <filesystem>

TEST_CASE("Parse binder_wcn.parm7", "[parm7]") {
  auto const data_dir = std::filesystem::path(RMS_TEST_DATA_DIR);
  auto const path = data_dir / "binder_wcn.parm7";

  auto topo = rms::parse_parm7_file(path);

  REQUIRE(topo.title == "default_name");
  REQUIRE(topo.pointers.natom == 13294);
  REQUIRE(topo.pointers.ntypes == 15);
  REQUIRE(topo.pointers.nres == 3321);

  REQUIRE(topo.atom_name.at(0) == "N1");
  REQUIRE(topo.atom_name.at(1) == "C1");
  REQUIRE(topo.residue_label.at(0) == "LIG");
  REQUIRE(topo.residue_pointer.at(0) == 0);

  REQUIRE(topo.mass.at(0) == Catch::Approx(14.01));
  REQUIRE(topo.charge.at(0) == Catch::Approx(-0.494686).margin(1e-6));

  auto const atom_to_res = rms::build_atom_residue_map(topo);
  REQUIRE(atom_to_res.at(0) == 0);

  auto const lj_idx = rms::lj_pair_index(topo, topo.atom_type_index.at(0), topo.atom_type_index.at(0));
  REQUIRE(lj_idx.has_value());
  REQUIRE(*lj_idx == 0);

  auto const lj_coeffs = rms::lj_pair_coeffs(topo, topo.atom_type_index.at(0), topo.atom_type_index.at(0));
  REQUIRE(lj_coeffs.has_value());
  REQUIRE(lj_coeffs->first == Catch::Approx(849322.032).margin(1e-3));
  REQUIRE(lj_coeffs->second == Catch::Approx(565.406768).margin(1e-3));

  REQUIRE(topo.nonbonded_parm_index.size() ==
    static_cast<std::size_t>(topo.pointers.ntypes) * static_cast<std::size_t>(topo.pointers.ntypes));
  REQUIRE(topo.lennard_jones_acoeff.size() ==
    static_cast<std::size_t>(topo.pointers.ntypes) * static_cast<std::size_t>(topo.pointers.ntypes + 1) / 2);

  REQUIRE(topo.bond_i.size() == static_cast<std::size_t>(topo.pointers.nbonh + topo.pointers.nbona));
  REQUIRE(topo.angle_i.size() == static_cast<std::size_t>(topo.pointers.ntheth + topo.pointers.ntheta));
  REQUIRE(topo.dihedral_i.size() == static_cast<std::size_t>(topo.pointers.nphih + topo.pointers.nphia));

  REQUIRE(topo.box_dimensions.has_value());
  REQUIRE((*topo.box_dimensions)[0] == Catch::Approx(109.471219));

  REQUIRE(topo.atoms_per_molecule.size() == 3321);
  REQUIRE(topo.atoms_per_molecule.at(0) == 47);
  REQUIRE(topo.radius_set == "modified Bondi radii (mbondi)");
}
