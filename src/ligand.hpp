/*

   Copyright (c) 2011, The Chinese University of Hong Kong

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#ifndef IDOCK_LIGAND_HPP
#define IDOCK_LIGAND_HPP

#include <boost/optional.hpp>
#include "atom.hpp"
#include "matrix.hpp"
#include "scoring_function.hpp"
#include "box.hpp"
#include "array3d.hpp"
#include "result.hpp"
#include "file.hpp"

namespace idock
{
	/// Represents a ROOT or a BRANCH in PDBQT structure.
	class frame
	{
	public:
		// These fields are essential to a frame.
		// They remain constant once initialized.
		size_t parent; ///< Frame array index pointing to the parent of current frame. For ROOT frame, this field is not used.
		size_t rotorX; ///< Index pointing to the parent frame atom which forms a rotatable bond with the first atom of current frame, a.k.a. rotor Y.
		bool active; ///< Indicates if the current frame is active.
		vector<atom> heavy_atoms; ///< Heavy atoms. Coordinates are relative to frame origin, which is the first atom by default.
		vector<atom> hydrogens; ///< Hydrogen atoms. Coordinates are relative to frame origin, which is the first atom by default.
		vec3 relative_origin; ///< Vector pointing from the origin of current frame to the origin of parent frame.
		vec3 relative_axis; ///< Normalized vector pointing from rotor Y to rotor X.

		// These fields are optional to a frame.
		// They vary from time to time during parsing or Monte Carlo simulation.
		vector<size_t> numbers; ///< Atom numbers.
		qt orientation_q; ///< Orientation in the form of quaternion.
		mat3 orientation_m; ///< Orientation in the form of 3x3 matrix.
		vec3 axis; ///< Vector pointing from rotor Y to rotor X.
		vec3 force; ///< Aggregated derivatives of heavy atoms.
		vec3 torque; /// Torque of the force.
		vector<vec3> coordinates; ///< Heavy atom coordinates.
		vector<vec3> derivatives; ///< Heavy atom derivatives.
		vector<fl> energies; ///< Heavy atom free energies.

		/// Constructs an active frame, and relates it to its parent frame.
		frame(const size_t parent) : parent(parent), active(true)
		{
			heavy_atoms.reserve(20); // A frame typically consists of < 20 heavy atoms.
			numbers.reserve(20); // A frame typically consists of < 20 heavy atoms.
			hydrogens.reserve(10); // A frame typically consists of < 20 hydrogen atoms.
		}
	};

	/// Represents a ligand.
	class ligand
	{
	public:
		vector<frame> frames; ///< ROOT and BRANCH frames.
		const vector<string> lines; ///< Input PDBQT file lines.
		const size_t num_frames; ///< Number of frames.
		const size_t num_torsions; ///< Number of torsions.
		const size_t num_active_torsions; ///< Number of active torsions.
		const size_t num_heavy_atoms; ///< Number of heavy atoms.
		const fl flexibility_penalty_factor; ///< A value in (0, 1] to penalize ligand flexibility.

		/// Constructs a ligand from frames and lines.
		ligand(vector<frame>&& frames_, vector<string>&& lines_, const size_t num_heavy_atoms, const size_t num_redundant_rotors);

		/// Returns the XScore atom types presented in current ligand.
		vector<size_t> get_atom_types() const;

		/// Evaluates free energy e, force f, and change g. Returns true if the conformation is accepted.
		bool evaluate(const conformation& conf, const scoring_function& sf, const box& b, const vector<array3d<fl> >& grid_maps, const fl e_upper_bound, fl& e, fl& f, change& g);

		/// Composes a result from free energy, force f, and conformation conf.
		result compose_result(const fl e, const fl f, const conformation& conf) const;

		/// Writes a given number of conformations from a result container into a output ligand file in PDBQT format.
		void write_models(const path& output_ligand, const ptr_vector<result>& result_cont, const size_t num_conformations);

	private:
		const fl num_heavy_atoms_inverse; ///< 1 / num_heavy_atoms.

		//class nonbonded_pair
		//{
		//public:
		//	size_t i;
		//	size_t j;
		//	fl square_sum_covalent_radius;
		//
		//	nonbonded_pair(const size_t i, const size_t j, const fl square_sum_covalent_radius) : i(i), j(j), square_sum_covalent_radius(square_sum_covalent_radius) {}
		//};

		//vector<nonbonded_pair> nonbonded_pairs;
	};
}

#endif