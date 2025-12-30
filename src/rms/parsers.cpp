#include "include/parsers.hpp"
#include "include/utils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace rms {
namespace {

struct FormatSpec {
  int count = 0;
  char type = '\0';
  int width = 0;
};

enum class Section {
  None,
  Unknown,
  Title,
  Pointers,
  AtomName,
  Charge,
  AtomicNumber,
  Mass,
  AtomTypeIndex,
  NumberExcludedAtoms,
  ExcludedAtomsList,
  NonbondedParmIndex,
  ResidueLabel,
  ResiduePointer,
  BondForceConstant,
  BondEquilValue,
  AngleForceConstant,
  AngleEquilValue,
  DihedralForceConstant,
  DihedralPeriodicity,
  DihedralPhase,
  SceeScaleFactor,
  ScnbScaleFactor,
  Solty,
  LennardJonesAcoef,
  LennardJonesBcoef,
  BondsIncHydrogen,
  BondsWithoutHydrogen,
  AnglesIncHydrogen,
  AnglesWithoutHydrogen,
  DihedralsIncHydrogen,
  DihedralsWithoutHydrogen,
  HbondAcoef,
  HbondBcoef,
  HbondCut,
  AmberAtomType,
  TreeChainClassification,
  JoinArray,
  Irotat,
  SolventPointers,
  AtomsPerMolecule,
  BoxDimensions,
  RadiusSet,
  Radii,
  Screen,
  Ipol
};

constexpr std::array<std::pair<std::string_view, Section>, 44> kSectionMap = {
  std::pair{"TITLE", Section::Title},
  std::pair{"POINTERS", Section::Pointers},
  std::pair{"ATOM_NAME", Section::AtomName},
  std::pair{"CHARGE", Section::Charge},
  std::pair{"ATOMIC_NUMBER", Section::AtomicNumber},
  std::pair{"MASS", Section::Mass},
  std::pair{"ATOM_TYPE_INDEX", Section::AtomTypeIndex},
  std::pair{"NUMBER_EXCLUDED_ATOMS", Section::NumberExcludedAtoms},
  std::pair{"EXCLUDED_ATOMS_LIST", Section::ExcludedAtomsList},
  std::pair{"NONBONDED_PARM_INDEX", Section::NonbondedParmIndex},
  std::pair{"RESIDUE_LABEL", Section::ResidueLabel},
  std::pair{"RESIDUE_POINTER", Section::ResiduePointer},
  std::pair{"BOND_FORCE_CONSTANT", Section::BondForceConstant},
  std::pair{"BOND_EQUIL_VALUE", Section::BondEquilValue},
  std::pair{"ANGLE_FORCE_CONSTANT", Section::AngleForceConstant},
  std::pair{"ANGLE_EQUIL_VALUE", Section::AngleEquilValue},
  std::pair{"DIHEDRAL_FORCE_CONSTANT", Section::DihedralForceConstant},
  std::pair{"DIHEDRAL_PERIODICITY", Section::DihedralPeriodicity},
  std::pair{"DIHEDRAL_PHASE", Section::DihedralPhase},
  std::pair{"SCEE_SCALE_FACTOR", Section::SceeScaleFactor},
  std::pair{"SCNB_SCALE_FACTOR", Section::ScnbScaleFactor},
  std::pair{"SOLTY", Section::Solty},
  std::pair{"LENNARD_JONES_ACOEF", Section::LennardJonesAcoef},
  std::pair{"LENNARD_JONES_BCOEF", Section::LennardJonesBcoef},
  std::pair{"BONDS_INC_HYDROGEN", Section::BondsIncHydrogen},
  std::pair{"BONDS_WITHOUT_HYDROGEN", Section::BondsWithoutHydrogen},
  std::pair{"ANGLES_INC_HYDROGEN", Section::AnglesIncHydrogen},
  std::pair{"ANGLES_WITHOUT_HYDROGEN", Section::AnglesWithoutHydrogen},
  std::pair{"DIHEDRALS_INC_HYDROGEN", Section::DihedralsIncHydrogen},
  std::pair{"DIHEDRALS_WITHOUT_HYDROGEN", Section::DihedralsWithoutHydrogen},
  std::pair{"HBOND_ACOEF", Section::HbondAcoef},
  std::pair{"HBOND_BCOEF", Section::HbondBcoef},
  std::pair{"HBCUT", Section::HbondCut},
  std::pair{"AMBER_ATOM_TYPE", Section::AmberAtomType},
  std::pair{"TREE_CHAIN_CLASSIFICATION", Section::TreeChainClassification},
  std::pair{"JOIN_ARRAY", Section::JoinArray},
  std::pair{"IROTAT", Section::Irotat},
  std::pair{"SOLVENT_POINTERS", Section::SolventPointers},
  std::pair{"ATOMS_PER_MOLECULE", Section::AtomsPerMolecule},
  std::pair{"BOX_DIMENSIONS", Section::BoxDimensions},
  std::pair{"RADIUS_SET", Section::RadiusSet},
  std::pair{"RADII", Section::Radii},
  std::pair{"SCREEN", Section::Screen},
  std::pair{"IPOL", Section::Ipol}
};

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) {
  return value.rfind(prefix, 0) == 0;
}

[[nodiscard]] FormatSpec parse_format_line(std::string_view line) {
  auto const open = line.find('(');
  auto const close = line.find(')', open == std::string_view::npos ? 0 : open + 1);
  if (open == std::string_view::npos || close == std::string_view::npos || close <= open + 1) {
    throw std::runtime_error(fmt::format("Invalid %FORMAT line: {}", line));
  }

  auto fmt = line.substr(open + 1, close - open - 1);
  std::size_t idx = 0;
  int count = 0;
  while (idx < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[idx]))) {
    count = count * 10 + (fmt[idx] - '0');
    ++idx;
  }
  if (count <= 0 || idx >= fmt.size()) {
    throw std::runtime_error(fmt::format("Invalid %FORMAT entry: {}", line));
  }

  char type = static_cast<char>(std::tolower(static_cast<unsigned char>(fmt[idx])));
  ++idx;

  int width = 0;
  while (idx < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[idx]))) {
    width = width * 10 + (fmt[idx] - '0');
    ++idx;
  }
  if (width <= 0) {
    throw std::runtime_error(fmt::format("Invalid %FORMAT width: {}", line));
  }

  return {count, type, width};
}

[[nodiscard]] Section parse_section_name(std::string_view line) {
  auto const name = trim(line.substr(6));
  for (auto const &[label, section] : kSectionMap) {
    if (name == label) {
      return section;
    }
  }
  return Section::Unknown;
}

[[nodiscard]] Parm7Pointers parse_pointers(const std::vector<int> &values) {
  if (values.size() < kParm7PointerCount) {
    throw std::runtime_error(fmt::format("POINTERS section has {} values, expected at least {}", values.size(),
      kParm7PointerCount));
  }

  Parm7Pointers ptr;
  ptr.natom = static_cast<std::uint16_t>(values[0]);
  ptr.ntypes = static_cast<std::uint16_t>(values[1]);
  ptr.nbonh = static_cast<std::uint16_t>(values[2]);
  ptr.mbona = static_cast<std::uint16_t>(values[3]);
  ptr.ntheth = static_cast<std::uint16_t>(values[4]);
  ptr.mtheta = static_cast<std::uint16_t>(values[5]);
  ptr.nphih = static_cast<std::uint16_t>(values[6]);
  ptr.mphia = static_cast<std::uint16_t>(values[7]);
  ptr.nhparm = static_cast<std::uint16_t>(values[8]);
  ptr.nparm = static_cast<std::uint16_t>(values[9]);
  ptr.nnb = static_cast<std::uint16_t>(values[10]);
  ptr.nres = static_cast<std::uint16_t>(values[11]);
  ptr.nbona = static_cast<std::uint16_t>(values[12]);
  ptr.ntheta = static_cast<std::uint16_t>(values[13]);
  ptr.nphia = static_cast<std::uint16_t>(values[14]);
  ptr.numbnd = static_cast<std::uint16_t>(values[15]);
  ptr.numang = static_cast<std::uint16_t>(values[16]);
  ptr.nptra = static_cast<std::uint16_t>(values[17]);
  ptr.natyp = static_cast<std::uint16_t>(values[18]);
  ptr.nphb = static_cast<std::uint16_t>(values[19]);
  ptr.ifpert = static_cast<std::uint16_t>(values[20]);
  ptr.nbper = static_cast<std::uint16_t>(values[21]);
  ptr.ngper = static_cast<std::uint16_t>(values[22]);
  ptr.ndper = static_cast<std::uint16_t>(values[23]);
  ptr.mbper = static_cast<std::uint16_t>(values[24]);
  ptr.mgper = static_cast<std::uint16_t>(values[25]);
  ptr.mdper = static_cast<std::uint16_t>(values[26]);
  ptr.ifbox = static_cast<std::uint16_t>(values[27]);
  ptr.nmxrs = static_cast<std::uint16_t>(values[28]);
  ptr.ifcap = static_cast<std::uint16_t>(values[29]);
  ptr.numextra = static_cast<std::uint16_t>(values[30]);
  if (values.size() > kParm7PointerCount) {
    ptr.ncopy = static_cast<std::uint16_t>(values[31]);
  }
  return ptr;
}

void reserve_from_pointers(Parm7Topology &topo, const Parm7Pointers &ptr) {
  auto const natom = static_cast<std::size_t>(ptr.natom);
  auto const nnb = static_cast<std::size_t>(ptr.nnb);
  auto const nres = static_cast<std::size_t>(ptr.nres);
  auto const numbnd = static_cast<std::size_t>(ptr.numbnd);
  auto const numang = static_cast<std::size_t>(ptr.numang);
  auto const nptra = static_cast<std::size_t>(ptr.nptra);
  auto const natyp = static_cast<std::size_t>(ptr.natyp);
  auto const ntypes = static_cast<std::size_t>(ptr.ntypes);
  auto const nphb = static_cast<std::size_t>(ptr.nphb);
  auto const bond_count = static_cast<std::size_t>(ptr.nbonh + ptr.nbona);
  auto const angle_count = static_cast<std::size_t>(ptr.ntheth + ptr.ntheta);
  auto const dihedral_count = static_cast<std::size_t>(ptr.nphih + ptr.nphia);

  topo.atom_name.reserve(natom);
  topo.charge.reserve(natom);
  topo.atomic_number.reserve(natom);
  topo.mass.reserve(natom);
  topo.atom_type_index.reserve(natom);
  topo.number_excluded_atoms.reserve(natom);
  topo.excluded_atoms_list.reserve(nnb);
  topo.nonbonded_parm_index.reserve(ntypes * ntypes);
  topo.residue_label.reserve(nres);
  topo.residue_pointer.reserve(nres);

  topo.bond_force_constant.reserve(numbnd);
  topo.bond_equil_value.reserve(numbnd);
  topo.angle_force_constant.reserve(numang);
  topo.angle_equil_value.reserve(numang);
  topo.dihedral_force_constant.reserve(nptra);
  topo.dihedral_periodicity.reserve(nptra);
  topo.dihedral_phase.reserve(nptra);
  topo.scee_scale_factor.reserve(nptra);
  topo.scnb_scale_factor.reserve(nptra);
  topo.solty.reserve(natyp);

  auto const lj_count = ntypes * (ntypes + 1U) / 2U;
  topo.lennard_jones_acoeff.reserve(lj_count);
  topo.lennard_jones_bcoeff.reserve(lj_count);

  topo.bond_i.reserve(bond_count);
  topo.bond_j.reserve(bond_count);
  topo.bond_type.reserve(bond_count);

  topo.angle_i.reserve(angle_count);
  topo.angle_j.reserve(angle_count);
  topo.angle_k.reserve(angle_count);
  topo.angle_type.reserve(angle_count);

  topo.dihedral_i.reserve(dihedral_count);
  topo.dihedral_j.reserve(dihedral_count);
  topo.dihedral_k.reserve(dihedral_count);
  topo.dihedral_l.reserve(dihedral_count);
  topo.dihedral_type.reserve(dihedral_count);
  topo.dihedral_flags.reserve(dihedral_count);

  topo.hbond_acoeff.reserve(nphb);
  topo.hbond_bcoeff.reserve(nphb);

  topo.amber_atom_type.reserve(natom);
  topo.tree_chain_classification.reserve(natom);
  topo.join_array.reserve(natom);
  topo.irotat.reserve(natom);

  topo.atoms_per_molecule.reserve(nres);
  topo.radii.reserve(natom);
  topo.screen.reserve(natom);
}

void append_strings(std::string_view line, const FormatSpec &fmt, std::vector<std::string> &out,
  std::optional<std::size_t> expected) {
  std::size_t const limit = expected.value_or(std::numeric_limits<std::size_t>::max());
  if (out.size() >= limit || fmt.width <= 0) {
    return;
  }

  std::size_t const width = static_cast<std::size_t>(fmt.width);
  std::size_t const max_fields = std::min<std::size_t>(static_cast<std::size_t>(fmt.count),
    std::max<std::size_t>(1, (line.size() + width - 1) / width));

  for (std::size_t idx = 0; idx < max_fields && out.size() < limit; ++idx) {
    std::size_t const start = idx * width;
    if (start >= line.size()) {
      break;
    }
    std::size_t const len = std::min<std::size_t>(width, line.size() - start);
    auto const raw = line.substr(start, len);
    out.emplace_back(trim(raw));
  }
}

template <typename Transform>
void append_ints_transform(std::string_view line, const FormatSpec &fmt, std::vector<int> &out,
  std::optional<std::size_t> expected, std::string_view section_name, Transform transform) {
  std::size_t const limit = expected.value_or(std::numeric_limits<std::size_t>::max());
  if (out.size() >= limit || fmt.width <= 0 || line.empty()) {
    return;
  }

  std::size_t const width = static_cast<std::size_t>(fmt.width);
  std::size_t const max_fields = std::min<std::size_t>(static_cast<std::size_t>(fmt.count),
    std::max<std::size_t>(1, (line.size() + width - 1) / width));

  for (std::size_t idx = 0; idx < max_fields && out.size() < limit; ++idx) {
    std::size_t const start = idx * width;
    if (start >= line.size()) {
      break;
    }
    std::size_t const len = std::min<std::size_t>(width, line.size() - start);
    auto const raw = line.substr(start, len);
    auto value = to_int(raw);
    if (!value) {
      if (trim(raw).empty()) {
        continue;
      }
      throw std::runtime_error(fmt::format("Failed to parse integer in {}: {}", section_name, raw));
    }
    out.push_back(transform(*value));
  }
}

template <typename Transform>
void append_doubles_transform(std::string_view line, const FormatSpec &fmt, std::vector<double> &out,
  std::optional<std::size_t> expected, std::string_view section_name, Transform transform) {
  std::size_t const limit = expected.value_or(std::numeric_limits<std::size_t>::max());
  if (out.size() >= limit || fmt.width <= 0 || line.empty()) {
    return;
  }

  std::size_t const width = static_cast<std::size_t>(fmt.width);
  std::size_t const max_fields = std::min<std::size_t>(static_cast<std::size_t>(fmt.count),
    std::max<std::size_t>(1, (line.size() + width - 1) / width));

  for (std::size_t idx = 0; idx < max_fields && out.size() < limit; ++idx) {
    std::size_t const start = idx * width;
    if (start >= line.size()) {
      break;
    }
    std::size_t const len = std::min<std::size_t>(width, line.size() - start);
    auto const raw = line.substr(start, len);
    auto value = to_double(raw);
    if (!value) {
      if (trim(raw).empty()) {
        continue;
      }
      throw std::runtime_error(fmt::format("Failed to parse float in {}: {}", section_name, raw));
    }
    out.push_back(transform(*value));
  }
}

void append_ints(std::string_view line, const FormatSpec &fmt, std::vector<int> &out,
  std::optional<std::size_t> expected, std::string_view section_name) {
  append_ints_transform(line, fmt, out, expected, section_name, [](int value) { return value; });
}

void append_doubles(std::string_view line, const FormatSpec &fmt, std::vector<double> &out,
  std::optional<std::size_t> expected, std::string_view section_name) {
  append_doubles_transform(line, fmt, out, expected, section_name, [](double value) { return value; });
}

void decode_bonds(const std::vector<int> &raw, Parm7Topology &topo) {
  if (raw.size() % 3 != 0) {
    throw std::runtime_error("Bond list size is not a multiple of 3");
  }
  for (std::size_t idx = 0; idx < raw.size(); idx += 3) {
    int const atom_i = raw[idx] / 3;
    int const atom_j = raw[idx + 1] / 3;
    int const type = raw[idx + 2] - 1;
    topo.bond_i.push_back(atom_i);
    topo.bond_j.push_back(atom_j);
    topo.bond_type.push_back(type);
  }
}

void decode_angles(const std::vector<int> &raw, Parm7Topology &topo) {
  if (raw.size() % 4 != 0) {
    throw std::runtime_error("Angle list size is not a multiple of 4");
  }
  for (std::size_t idx = 0; idx < raw.size(); idx += 4) {
    int const atom_i = raw[idx] / 3;
    int const atom_j = raw[idx + 1] / 3;
    int const atom_k = raw[idx + 2] / 3;
    int const type = raw[idx + 3] - 1;
    topo.angle_i.push_back(atom_i);
    topo.angle_j.push_back(atom_j);
    topo.angle_k.push_back(atom_k);
    topo.angle_type.push_back(type);
  }
}

void decode_dihedrals(const std::vector<int> &raw, Parm7Topology &topo) {
  if (raw.size() % 5 != 0) {
    throw std::runtime_error("Dihedral list size is not a multiple of 5");
  }
  for (std::size_t idx = 0; idx < raw.size(); idx += 5) {
    int const raw_i = raw[idx];
    int const raw_j = raw[idx + 1];
    int const raw_k = raw[idx + 2];
    int const raw_l = raw[idx + 3];
    int const type = raw[idx + 4] - 1;

    bool const suppress_14 = raw_k < 0;
    bool const improper = raw_l < 0;

    int const atom_i = raw_i / 3;
    int const atom_j = raw_j / 3;
    int const atom_k = std::abs(raw_k) / 3;
    int const atom_l = std::abs(raw_l) / 3;

    std::uint8_t flags = 0;
    if (suppress_14) {
      flags |= 0x1u;
    }
    if (improper) {
      flags |= 0x2u;
    }

    topo.dihedral_i.push_back(atom_i);
    topo.dihedral_j.push_back(atom_j);
    topo.dihedral_k.push_back(atom_k);
    topo.dihedral_l.push_back(atom_l);
    topo.dihedral_type.push_back(type);
    topo.dihedral_flags.push_back(flags);
  }
}

void require_size(std::string_view name, std::size_t actual, std::size_t expected) {
  if (actual != expected) {
    throw std::runtime_error(fmt::format("Section {} has {} entries, expected {}", name, actual, expected));
  }
}

} // namespace

Parm7Topology parse_parm7_file(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error(fmt::format("Failed to open parm7 file: {}", path.string()));
  }

  Parm7Topology topo;
  Section current_section = Section::None;
  FormatSpec current_format{};

  std::vector<int> pointer_values;
  std::vector<int> bonds_inc_raw;
  std::vector<int> bonds_noh_raw;
  std::vector<int> angles_inc_raw;
  std::vector<int> angles_noh_raw;
  std::vector<int> dihedrals_inc_raw;
  std::vector<int> dihedrals_noh_raw;
  std::vector<double> hbond_cut_raw;
  std::vector<int> solvent_pointer_raw;
  std::vector<double> box_dimensions_raw;

  bool pointers_ready = false;

  std::string line;
  while (std::getline(file, line)) {
    std::string_view line_view(line);

    if (starts_with(line_view, "%VERSION")) {
      topo.version = std::string(trim(line_view));
      continue;
    }

    if (starts_with(line_view, "%FLAG")) {
      if (current_section == Section::Pointers && !pointers_ready) {
        topo.pointers = parse_pointers(pointer_values);
        reserve_from_pointers(topo, topo.pointers);
        pointers_ready = true;
      }

      current_section = parse_section_name(line_view);
      if (!std::getline(file, line)) {
        throw std::runtime_error("Unexpected end of file after %FLAG line");
      }
      current_format = parse_format_line(line);
      continue;
    }

    if (current_section == Section::None || current_section == Section::Unknown) {
      continue;
    }

    switch (current_section) {
      case Section::Title:
        topo.title.append(line);
        break;
      case Section::Pointers:
        append_ints(line_view, current_format, pointer_values, std::nullopt, "POINTERS");
        break;
      case Section::AtomName:
        append_strings(line_view, current_format, topo.atom_name,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt);
        break;
      case Section::Charge:
        append_doubles_transform(line_view, current_format, topo.charge,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "CHARGE",
          [](double value) { return value / kAmberChargeScale; });
        break;
      case Section::AtomicNumber:
        append_ints(line_view, current_format, topo.atomic_number,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt,
          "ATOMIC_NUMBER");
        break;
      case Section::Mass:
        append_doubles(line_view, current_format, topo.mass,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "MASS");
        break;
      case Section::AtomTypeIndex:
        append_ints_transform(line_view, current_format, topo.atom_type_index,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "ATOM_TYPE_INDEX",
          [](int value) { return value - 1; });
        break;
      case Section::NumberExcludedAtoms:
        append_ints(line_view, current_format, topo.number_excluded_atoms,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt,
          "NUMBER_EXCLUDED_ATOMS");
        break;
      case Section::ExcludedAtomsList:
        append_ints_transform(line_view, current_format, topo.excluded_atoms_list,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nnb) : std::nullopt, "EXCLUDED_ATOMS_LIST",
          [](int value) { return value == 0 ? -1 : value - 1; });
        break;
      case Section::NonbondedParmIndex:
        append_ints_transform(line_view, current_format, topo.nonbonded_parm_index,
          pointers_ready ? std::optional<std::size_t>(
                             static_cast<std::size_t>(topo.pointers.ntypes) *
                             static_cast<std::size_t>(topo.pointers.ntypes))
                         : std::nullopt,
          "NONBONDED_PARM_INDEX", [](int value) { return value == 0 ? -1 : value - 1; });
        break;
      case Section::ResidueLabel:
        append_strings(line_view, current_format, topo.residue_label,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nres) : std::nullopt);
        break;
      case Section::ResiduePointer:
        append_ints_transform(line_view, current_format, topo.residue_pointer,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nres) : std::nullopt, "RESIDUE_POINTER",
          [](int value) { return value - 1; });
        break;
      case Section::BondForceConstant:
        append_doubles(line_view, current_format, topo.bond_force_constant,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.numbnd) : std::nullopt,
          "BOND_FORCE_CONSTANT");
        break;
      case Section::BondEquilValue:
        append_doubles(line_view, current_format, topo.bond_equil_value,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.numbnd) : std::nullopt,
          "BOND_EQUIL_VALUE");
        break;
      case Section::AngleForceConstant:
        append_doubles(line_view, current_format, topo.angle_force_constant,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.numang) : std::nullopt,
          "ANGLE_FORCE_CONSTANT");
        break;
      case Section::AngleEquilValue:
        append_doubles(line_view, current_format, topo.angle_equil_value,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.numang) : std::nullopt,
          "ANGLE_EQUIL_VALUE");
        break;
      case Section::DihedralForceConstant:
        append_doubles(line_view, current_format, topo.dihedral_force_constant,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nptra) : std::nullopt,
          "DIHEDRAL_FORCE_CONSTANT");
        break;
      case Section::DihedralPeriodicity:
        append_doubles(line_view, current_format, topo.dihedral_periodicity,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nptra) : std::nullopt,
          "DIHEDRAL_PERIODICITY");
        break;
      case Section::DihedralPhase:
        append_doubles(line_view, current_format, topo.dihedral_phase,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nptra) : std::nullopt,
          "DIHEDRAL_PHASE");
        break;
      case Section::SceeScaleFactor:
        append_doubles(line_view, current_format, topo.scee_scale_factor,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nptra) : std::nullopt,
          "SCEE_SCALE_FACTOR");
        break;
      case Section::ScnbScaleFactor:
        append_doubles(line_view, current_format, topo.scnb_scale_factor,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nptra) : std::nullopt,
          "SCNB_SCALE_FACTOR");
        break;
      case Section::Solty:
        append_doubles(line_view, current_format, topo.solty,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natyp) : std::nullopt, "SOLTY");
        break;
      case Section::LennardJonesAcoef:
        append_doubles(line_view, current_format, topo.lennard_jones_acoeff,
          pointers_ready ? std::optional<std::size_t>(
                             static_cast<std::size_t>(topo.pointers.ntypes) *
                             static_cast<std::size_t>(topo.pointers.ntypes + 1) / 2)
                         : std::nullopt,
          "LENNARD_JONES_ACOEF");
        break;
      case Section::LennardJonesBcoef:
        append_doubles(line_view, current_format, topo.lennard_jones_bcoeff,
          pointers_ready ? std::optional<std::size_t>(
                             static_cast<std::size_t>(topo.pointers.ntypes) *
                             static_cast<std::size_t>(topo.pointers.ntypes + 1) / 2)
                         : std::nullopt,
          "LENNARD_JONES_BCOEF");
        break;
      case Section::BondsIncHydrogen:
        append_ints(line_view, current_format, bonds_inc_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.nbonh) * 3)
                         : std::nullopt,
          "BONDS_INC_HYDROGEN");
        break;
      case Section::BondsWithoutHydrogen:
        append_ints(line_view, current_format, bonds_noh_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.nbona) * 3)
                         : std::nullopt,
          "BONDS_WITHOUT_HYDROGEN");
        break;
      case Section::AnglesIncHydrogen:
        append_ints(line_view, current_format, angles_inc_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.ntheth) * 4)
                         : std::nullopt,
          "ANGLES_INC_HYDROGEN");
        break;
      case Section::AnglesWithoutHydrogen:
        append_ints(line_view, current_format, angles_noh_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.ntheta) * 4)
                         : std::nullopt,
          "ANGLES_WITHOUT_HYDROGEN");
        break;
      case Section::DihedralsIncHydrogen:
        append_ints(line_view, current_format, dihedrals_inc_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.nphih) * 5)
                         : std::nullopt,
          "DIHEDRALS_INC_HYDROGEN");
        break;
      case Section::DihedralsWithoutHydrogen:
        append_ints(line_view, current_format, dihedrals_noh_raw,
          pointers_ready ? std::optional<std::size_t>(static_cast<std::size_t>(topo.pointers.nphia) * 5)
                         : std::nullopt,
          "DIHEDRALS_WITHOUT_HYDROGEN");
        break;
      case Section::HbondAcoef:
        append_doubles(line_view, current_format, topo.hbond_acoeff,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nphb) : std::nullopt, "HBOND_ACOEF");
        break;
      case Section::HbondBcoef:
        append_doubles(line_view, current_format, topo.hbond_bcoeff,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nphb) : std::nullopt, "HBOND_BCOEF");
        break;
      case Section::HbondCut:
        append_doubles(line_view, current_format, hbond_cut_raw, std::nullopt, "HBCUT");
        break;
      case Section::AmberAtomType:
        append_strings(line_view, current_format, topo.amber_atom_type,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt);
        break;
      case Section::TreeChainClassification:
        append_strings(line_view, current_format, topo.tree_chain_classification,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt);
        break;
      case Section::JoinArray:
        append_ints(line_view, current_format, topo.join_array,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "JOIN_ARRAY");
        break;
      case Section::Irotat:
        append_ints(line_view, current_format, topo.irotat,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "IROTAT");
        break;
      case Section::SolventPointers:
        append_ints(line_view, current_format, solvent_pointer_raw, std::nullopt, "SOLVENT_POINTERS");
        break;
      case Section::AtomsPerMolecule:
        append_ints(line_view, current_format, topo.atoms_per_molecule,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.nres) : std::nullopt, "ATOMS_PER_MOLECULE");
        break;
      case Section::BoxDimensions:
        append_doubles(line_view, current_format, box_dimensions_raw, std::nullopt, "BOX_DIMENSIONS");
        break;
      case Section::RadiusSet:
        if (topo.radius_set.empty()) {
          topo.radius_set = std::string(trim(line_view));
        }
        break;
      case Section::Radii:
        append_doubles(line_view, current_format, topo.radii,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "RADII");
        break;
      case Section::Screen:
        append_doubles(line_view, current_format, topo.screen,
          pointers_ready ? std::optional<std::size_t>(topo.pointers.natom) : std::nullopt, "SCREEN");
        break;
      case Section::Ipol: {
        std::vector<int> raw;
        append_ints(line_view, current_format, raw, std::nullopt, "IPOL");
        if (!raw.empty()) {
          topo.ipol = raw.front();
        }
      } break;
      default:
        break;
    }
  }

  if (!pointers_ready) {
    topo.pointers = parse_pointers(pointer_values);
    reserve_from_pointers(topo, topo.pointers);
  }

  if (!topo.title.empty()) {
    auto const trimmed = trim(std::string_view(topo.title));
    topo.title.assign(trimmed);
  }

  if (!solvent_pointer_raw.empty()) {
    if (solvent_pointer_raw.size() < 3) {
      throw std::runtime_error("SOLVENT_POINTERS section has fewer than 3 values");
    }
    topo.solvent_pointers = std::array<int, 3>{solvent_pointer_raw[0], solvent_pointer_raw[1], solvent_pointer_raw[2]};
  }

  if (!box_dimensions_raw.empty()) {
    if (box_dimensions_raw.size() < 4) {
      throw std::runtime_error("BOX_DIMENSIONS section has fewer than 4 values");
    }
    topo.box_dimensions = std::array<double, 4>{box_dimensions_raw[0], box_dimensions_raw[1], box_dimensions_raw[2],
      box_dimensions_raw[3]};
  }

  decode_bonds(bonds_inc_raw, topo);
  decode_bonds(bonds_noh_raw, topo);
  decode_angles(angles_inc_raw, topo);
  decode_angles(angles_noh_raw, topo);
  decode_dihedrals(dihedrals_inc_raw, topo);
  decode_dihedrals(dihedrals_noh_raw, topo);

  if (!hbond_cut_raw.empty()) {
    topo.hbond_cut = hbond_cut_raw.front();
  }

  auto const natom = static_cast<std::size_t>(topo.pointers.natom);
  auto const nnb = static_cast<std::size_t>(topo.pointers.nnb);
  auto const nres = static_cast<std::size_t>(topo.pointers.nres);
  auto const numbnd = static_cast<std::size_t>(topo.pointers.numbnd);
  auto const numang = static_cast<std::size_t>(topo.pointers.numang);
  auto const nptra = static_cast<std::size_t>(topo.pointers.nptra);
  auto const natyp = static_cast<std::size_t>(topo.pointers.natyp);
  auto const ntypes = static_cast<std::size_t>(topo.pointers.ntypes);
  auto const nphb = static_cast<std::size_t>(topo.pointers.nphb);
  auto const bond_count = static_cast<std::size_t>(topo.pointers.nbonh + topo.pointers.nbona);
  auto const angle_count = static_cast<std::size_t>(topo.pointers.ntheth + topo.pointers.ntheta);
  auto const dihedral_count = static_cast<std::size_t>(topo.pointers.nphih + topo.pointers.nphia);

  require_size("ATOM_NAME", topo.atom_name.size(), natom);
  require_size("CHARGE", topo.charge.size(), natom);
  require_size("ATOMIC_NUMBER", topo.atomic_number.size(), natom);
  require_size("MASS", topo.mass.size(), natom);
  require_size("ATOM_TYPE_INDEX", topo.atom_type_index.size(), natom);
  require_size("NUMBER_EXCLUDED_ATOMS", topo.number_excluded_atoms.size(), natom);
  require_size("EXCLUDED_ATOMS_LIST", topo.excluded_atoms_list.size(), nnb);
  require_size("NONBONDED_PARM_INDEX", topo.nonbonded_parm_index.size(), ntypes * ntypes);
  require_size("RESIDUE_LABEL", topo.residue_label.size(), nres);
  require_size("RESIDUE_POINTER", topo.residue_pointer.size(), nres);
  require_size("BOND_FORCE_CONSTANT", topo.bond_force_constant.size(), numbnd);
  require_size("BOND_EQUIL_VALUE", topo.bond_equil_value.size(), numbnd);
  require_size("ANGLE_FORCE_CONSTANT", topo.angle_force_constant.size(), numang);
  require_size("ANGLE_EQUIL_VALUE", topo.angle_equil_value.size(), numang);
  require_size("DIHEDRAL_FORCE_CONSTANT", topo.dihedral_force_constant.size(), nptra);
  require_size("DIHEDRAL_PERIODICITY", topo.dihedral_periodicity.size(), nptra);
  require_size("DIHEDRAL_PHASE", topo.dihedral_phase.size(), nptra);
  require_size("SCEE_SCALE_FACTOR", topo.scee_scale_factor.size(), nptra);
  require_size("SCNB_SCALE_FACTOR", topo.scnb_scale_factor.size(), nptra);
  require_size("SOLTY", topo.solty.size(), natyp);

  auto const lj_count = ntypes * (ntypes + 1U) / 2U;
  require_size("LENNARD_JONES_ACOEF", topo.lennard_jones_acoeff.size(), lj_count);
  require_size("LENNARD_JONES_BCOEF", topo.lennard_jones_bcoeff.size(), lj_count);

  require_size("BONDS", topo.bond_i.size(), bond_count);
  require_size("ANGLES", topo.angle_i.size(), angle_count);
  require_size("DIHEDRALS", topo.dihedral_i.size(), dihedral_count);

  if (topo.pointers.nphb > 0) {
    require_size("HBOND_ACOEF", topo.hbond_acoeff.size(), nphb);
    require_size("HBOND_BCOEF", topo.hbond_bcoeff.size(), nphb);
    if (!topo.hbond_cut) {
      throw std::runtime_error("HBCUT missing but NPHB > 0");
    }
  }

  require_size("AMBER_ATOM_TYPE", topo.amber_atom_type.size(), natom);
  require_size("TREE_CHAIN_CLASSIFICATION", topo.tree_chain_classification.size(), natom);
  require_size("JOIN_ARRAY", topo.join_array.size(), natom);
  require_size("IROTAT", topo.irotat.size(), natom);

  if (topo.pointers.ifbox > 0 && !topo.box_dimensions) {
    throw std::runtime_error("BOX_DIMENSIONS missing but IFBOX > 0");
  }

  require_size("RADII", topo.radii.size(), natom);
  require_size("SCREEN", topo.screen.size(), natom);

  if (!topo.atoms_per_molecule.empty()) {
    require_size("ATOMS_PER_MOLECULE", topo.atoms_per_molecule.size(), nres);
  }

  return topo;
}

} // namespace rms
