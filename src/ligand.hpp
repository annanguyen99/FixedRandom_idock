#pragma once
#ifndef IDOCK_LIGAND_HPP
#define IDOCK_LIGAND_HPP

#include <boost/filesystem/path.hpp>
#include "scoring_function.hpp"
#include "random_forest.hpp"
#include "atom.hpp"
#include "receptor.hpp"
#include "conformation.hpp"
#include "result.hpp"
using namespace boost::filesystem;

//! Represents a ROOT or a BRANCH in PDBQT structure.
class frame
{
public:
	size_t parent; //!< Frame array index pointing to the parent of current frame. For ROOT frame, this field is not used.
	size_t rotorXsrn; //!< Serial atom number of the parent frame atom which forms a rotatable bond with the rotorY atom of current frame.
	size_t rotorYsrn; //!< Serial atom number of the current frame atom which forms a rotatable bond with the rotorX atom of parent frame.
	size_t rotorXidx; //!< Index pointing to the parent frame atom which forms a rotatable bond with the rotorY atom of current frame.
	size_t rotorYidx; //!< Index pointing to the current frame atom which forms a rotatable bond with the rotorX atom of parent frame.
	size_t habegin; //!< The inclusive beginning index to the heavy atoms of the current frame.
	size_t haend; //!< The exclusive ending index to the heavy atoms of the current frame.
	size_t hybegin; //!< The inclusive beginning index to the hydrogen atoms of the current frame.
	size_t hyend; //!< The exclusive ending index to the hydrogen atoms of the current frame.
	bool active; //!< Indicates if the current frame is active.
	array<double, 3> parent_rotorY_to_current_rotorY; //!< Vector pointing from the origin of parent frame to the origin of current frame.
	array<double, 3> parent_rotorX_to_current_rotorY; //!< Normalized vector pointing from rotor X of parent frame to rotor Y of current frame.

	//! Constructs an active frame, and relates it to its parent frame.
	explicit frame(const size_t parent, const size_t rotorXsrn, const size_t rotorYsrn, const size_t rotorXidx, const size_t habegin, const size_t hybegin) : parent(parent), rotorXsrn(rotorXsrn), rotorYsrn(rotorYsrn), rotorXidx(rotorXidx), habegin(habegin), hybegin(hybegin), active(true) {}
};

//! Represents a ligand.
class ligand
{
public:
	vector<string> lines; //!< Input PDBQT file lines.
	vector<frame> frames; //!< ROOT and BRANCH frames.
	vector<atom> heavy_atoms; //!< Heavy atoms. Coordinates are relative to frame origin, which is the first atom by default.
	vector<atom> hydrogens; //!< Hydrogen atoms. Coordinates are relative to frame origin, which is the first atom by default.
	array<bool, scoring_function::n> xs; //!< Presence of XScore atom types.
	size_t num_heavy_atoms; //!< Number of heavy atoms.
	size_t num_hydrogens; //!< Number of hydrogens.
	size_t num_frames; //!< Number of frames.
	size_t num_torsions; //!< Number of torsions.
	size_t num_active_torsions; //!< Number of active torsions.
	double flexibility_penalty_factor; //!< A value in (0, 1] to penalize ligand flexibility.

	//! Constructs a ligand by parsing a ligand file in pdbqt format.
	//! @exception parsing_error Thrown when an atom type is not recognized or an empty branch is detected.
	ligand(const path& p);

	//! Evaluates free energy e, force f, and change g. Returns true if the conformation is accepted.
	bool evaluate(const conformation& conf, const scoring_function& sf, const receptor& rec, const double e_upper_bound, double& e, double& f, change& g) const;

	//! Composes a result from free energy, inter-molecular free energy f, and conformation conf.
	result compose_result(const double e, const double f, const conformation& conf) const;

	//! Writes a given number of conformations from a result container into a output ligand file in PDBQT format.
	void write_models(const path& output_ligand_path, const ptr_vector<result>& results, const size_t num_conformations, const receptor& rec, const forest& f, const scoring_function& sf);

	void monte_carlo(ptr_vector<result>& results, const size_t seed, const scoring_function& sf, const receptor& rec) const;

private:
	//! Represents a pair of interacting atoms that are separated by 3 consecutive covalent bonds.
	class interacting_pair
	{
	public:
		size_t i0; //!< Index of atom 0.
		size_t i1; //!< Index of atom 1.
		size_t p_offset; //!< Index to the XScore types of the two atoms for fast evaluating the scoring function.
		interacting_pair(const size_t i0, const size_t i1, const size_t p_offset) : i0(i0), i1(i1), p_offset(p_offset) {}
	};

	vector<interacting_pair> interacting_pairs; //!< Non 1-4 interacting pairs.
};

#endif
