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

#include "fstream.hpp"
#include "parsing_error.hpp"
#include "ligand.hpp"

namespace idock
{
	ligand::ligand(const path& p) : num_active_torsions(0), num_heavy_atoms(0)
	{
		// Initialize necessary variables for constructing a ligand.
		lines.reserve(200); // A ligand typically consists of <= 200 lines.
		frames.reserve(30); // A ligand typically consists of <= 30 frames.
		frames.push_back(frame(0, 0)); // ROOT is also treated as a frame. The parent and rotorX of ROOT frame are dummy.

		// Initialize helper variables for parsing.
		size_t current = 0; // Index of current frame, initialized to ROOT frame.
		frame* f = &frames.front(); // Pointer to the current frame.
		size_t num_lines = 0; // Used to track line number for reporting parsing errors, if any.
		string line;
		line.reserve(79); // According to PDBQT specification, the last item AutoDock atom type locates at 1-based [78, 79].

		// Parse ROOT, ATOM/HETATM, ENDROOT, BRANCH, ENDBRANCH, TORSDOF.
		ifstream in(p); // Parsing starts. Open the file stream as late as possible.
		while (getline(in, line))
		{
			++num_lines;
			if (starts_with(line, "ATOM") || starts_with(line, "HETATM"))
			{
				// Whenever an ATOM/HETATM line shows up, the current frame must be the last one.
				BOOST_ASSERT(current == frames.size() - 1);
				BOOST_ASSERT(f == &frames.back());

				// This line will be dumped to the output ligand file.
				lines.push_back(line);

				// Parse and validate AutoDock4 atom type.
				const string ad_type_string = line.substr(77, isspace(line[78]) ? 1 : 2);
				const size_t ad = parse_ad_type_string(ad_type_string);
				if (ad == AD_TYPE_SIZE) throw parsing_error(p, num_lines, "Atom type " + ad_type_string + " is not supported by idock.");

				// Parse the Cartesian coordinate.
				const atom a(vec3(right_cast<fl>(line, 31, 38), right_cast<fl>(line, 39, 46), right_cast<fl>(line, 47, 54)), ad);

				// For a hydrogen, save it.
				if (a.is_hydrogen())
				{
					f->hydrogens.push_back(a);

					// For a polar hydrogen, the bonded hetero atom must be a hydrogen bond donor.
					if (ad == AD_TYPE_HD)
					{
						for (size_t i = f->heavy_atoms.size(); i > 0;)
						{
							atom& b = f->heavy_atoms[--i];
							if (!b.is_hetero()) continue; // Only a hetero atom can be a hydrogen bond donor.
							if (a.is_neighbor(b))
							{
								b.donorize();
								break;
							}
						}
					}
				}
				else // It is a heavy atom.
				{
					f->heavy_atoms.push_back(a);
					f->numbers.push_back(right_cast<size_t>(line, 7, 11));
					++num_heavy_atoms;
				}
			}
			else if (starts_with(line, "BRANCH"))
			{
				// This line will be dumped to the output ligand file.
				lines.push_back(line);

				// Parse "BRANCH   X   Y". X and Y are right-justified and 4 characters wide.
				// Y is not parsed because the atom immediately follows "BRANCH" must be Y in pdbqt files created by the prepare_ligand4.py script of MGLTools.
				// This assumption fails if pdbqt files are prepared by OpenBabel. In this case, the class frame should incorporate a new field rotorY to track the origin.
				const size_t x = right_cast<size_t>(line, 7, 10);

				// Find the corresponding heavy atom with X as its atom number in the current frame.
				for (size_t i = 0; i < f->numbers.size(); ++i)
				{
					if (f->numbers[i] == x)
					{
						// Insert a new frame whose parent is the current frame.
						frames.push_back(frame(current, i));
						break;
					}
				}

				// Now the current frame is the newly inserted BRANCH frame.
				current = frames.size() - 1;

				// Update the pointer to the current frame.
				f = &frames[current];
			}
			else if (starts_with(line, "ENDBRANCH"))
			{
				// This line will be dumped to the output ligand file.
				lines.push_back(line);

				// A frame may be empty, e.g. "BRANCH   4   9" is immediately followed by "ENDBRANCH   4   9".
				// This emptiness is likely to be caused by invalid input structure, especially when all the atoms are located in the same plane.
				if (f->heavy_atoms.empty()) throw parsing_error(p, num_lines, "An empty BRANCH has been detected, indicating the input ligand structure is probably invalid.");

				// If the current frame consists of rotor Y and a few hydrogens only, e.g. -OH and -NH2,
				// the torsion of this frame will have no effect on scoring and is thus redundant.
				if ((current == frames.size() - 1) && (f->heavy_atoms.size() == 1))
				{
					f->active = false;
				}
				else
				{
					++num_active_torsions;
				}

				// Now the parent of the following frame is the parent of current frame.
				current = frames[current].parent;

				// Update the pointer to the current frame.
				f = &frames[current];
			}
			else if (starts_with(line, "ROOT") || starts_with(line, "ENDROOT") || starts_with(line, "TORSDOF"))
			{
				// This line will be dumped to the output ligand file.
				lines.push_back(line);
			}
		}
		in.close(); // Parsing finishes. Close the file stream as soon as possible.

		BOOST_ASSERT(current == 0); // current should remain its original value if "BRANCH" and "ENDBRANCH" properly match each other.
		BOOST_ASSERT(f == &frames.front()); // f should remain its original value if "BRANCH" and "ENDBRANCH" properly match each other.
		BOOST_ASSERT(lines.size() <= num_lines); // Some lines like "REMARK", "WARNING", "TER" will not be dumped to the output ligand file.

		// Determine num_frames, num_torsions, flexibility_penalty_factor, and num_heavy_atoms_inverse.
		num_frames = frames.size();
		BOOST_ASSERT(num_frames >= 1);
		num_torsions = num_frames - 1;
		BOOST_ASSERT(num_torsions + 1 == num_frames);
		BOOST_ASSERT(num_torsions >= num_active_torsions);
		flexibility_penalty_factor = 1 / (1 + 0.05846 * (num_active_torsions + 0.5 * (num_torsions - num_active_torsions)));
		num_heavy_atoms_inverse = static_cast<fl>(1) / num_heavy_atoms;

		// Dehydrophobicize carbons if necessary.
		for (size_t k = 0; k < num_frames; ++k)
		{
			frame& f = frames[k];
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				const atom& a = f.heavy_atoms[i];
				if (!a.is_hetero()) continue; // a is a hetero atom.

				for (size_t j = 0; j < num_heavy_atoms; ++j)
				{
					atom& b = f.heavy_atoms[j];
					if (b.is_hetero()) continue; // b is a carbon atom.

					// If the carbon atom b is bonded to the hetero atom a, it is no longer a hydrophobic atom.
					if (a.is_neighbor(b))
					{
						b.dehydrophobicize();
					}
				}
			}

			if (k > 0)
			{
				atom& rotorY = f.heavy_atoms.front();
				atom& rotorX = frames[f.parent].heavy_atoms[f.rotorX];
				if ((rotorY.is_hetero()) && (!rotorX.is_hetero())) rotorX.dehydrophobicize();
				if ((rotorX.is_hetero()) && (!rotorY.is_hetero())) rotorY.dehydrophobicize();
			}
		}

		// Initialize relative_origin and relative_axis of BRANCH frames.
		for (size_t k = 1; k < num_frames; ++k)
		{
			frame& f = frames[k];
			const frame& pf = frames[f.parent];
			const vec3& origin = f.heavy_atoms.front().coordinate;
			f.relative_origin =  origin - pf.heavy_atoms.front().coordinate;
			f.relative_axis   = (origin - pf.heavy_atoms[f.rotorX].coordinate).normalize();
		}

		// Reserve enough capacity for bonds.
		using std::pair;
		vector<vector<vector<pair<size_t, size_t>>>> bonds(num_frames);
		for (size_t k = 0; k < num_frames; ++k)
		{
			const frame& f = frames[k];
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			bonds[k].resize(num_heavy_atoms);
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				bonds[k][i].reserve(4); // An atom typically consists of <= 4 bonds.
			}
		}

		for (size_t k = 0; k < num_frames; ++k)
		{
			const frame& f = frames[k];
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				const atom& a1 = f.heavy_atoms[i];

				for (size_t j = i + 1; j < num_heavy_atoms; ++j)
				{
					const atom& a2 = f.heavy_atoms[j];
					if (a1.is_neighbor(a2))
					{
						bonds[k][i].push_back(pair<size_t, size_t>(k, j));
						bonds[k][j].push_back(pair<size_t, size_t>(k, i));
					}
				}
			}
			if (k > 0)
			{
				bonds[k][0].push_back(pair<size_t, size_t>(f.parent, f.rotorX));
				bonds[f.parent][f.rotorX].push_back(pair<size_t, size_t>(k, 0));
			}
		}

		// Find 1-4 interacting pairs.
		one_to_four_pairs.reserve(num_heavy_atoms * num_heavy_atoms);
		vector<pair<size_t, size_t>> neighbors;
		neighbors.reserve(10); // An atom typically consists of <= 10 neighbors.
		for (size_t k1 = 0; k1 < num_frames; ++k1)
		{
			const frame& f1 = frames[k1];
			const size_t num_heavy_atoms1 = f1.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms1; ++i)
			{
				// Find neighbor atoms within 3 consecutive covalent bonds.
				const vector<pair<size_t, size_t>>& i0_bonds = bonds[k1][i];
				const size_t num_i0_bonds = i0_bonds.size();
				for (size_t i0 = 0; i0 < num_i0_bonds; ++i0)
				{
					const pair<size_t, size_t>& b1 = i0_bonds[i0];
					if (find(neighbors.begin(), neighbors.end(), b1) == neighbors.end())
					{
						neighbors.push_back(b1);
					}
					const vector<pair<size_t, size_t>>& i1_bonds = bonds[b1.first][b1.second];
					const size_t num_i1_bonds = i1_bonds.size();
					for (size_t i1 = 0; i1 < num_i1_bonds; ++i1)
					{
						const pair<size_t, size_t>& b2 = i1_bonds[i1];
						if (find(neighbors.begin(), neighbors.end(), b2) == neighbors.end())
						{
							neighbors.push_back(b2);
						}
						const vector<pair<size_t, size_t>>& i2_bonds = bonds[b2.first][b2.second];
						const size_t num_i2_bonds = i2_bonds.size();
						for (size_t i2 = 0; i2 < num_i2_bonds; ++i2)
						{
							const pair<size_t, size_t>& b3 = i2_bonds[i2];
							if (find(neighbors.begin(), neighbors.end(), b3) == neighbors.end())
							{
								neighbors.push_back(b3);
							}
						}
					}
				}

				// Determine if interacting pairs can be possibly formed.
				for (size_t k2 = k1 + 1; k2 < num_frames; ++k2)
				{
					const frame& f2 = frames[k2];
					const size_t num_heavy_atoms2 = f2.heavy_atoms.size();
					for (size_t j = 0; j < num_heavy_atoms2; ++j)
					{
						if (((k1 == f2.parent) && ((j == 0) || (i == f2.rotorX))) || (find(neighbors.begin(), neighbors.end(), pair<size_t, size_t>(k2, j)) != neighbors.end())) continue;
						const size_t type_pair_index = triangular_matrix_permissive_index(f1.heavy_atoms[i].xs, f2.heavy_atoms[j].xs);
						one_to_four_pairs.push_back(one_to_four_pair(k1, i, k2, j, type_pair_index));
					}
				}

				// Clear the current neighbor set for the next atom.
				neighbors.clear();
			}
		}

		// Update heavy_atoms[].coordinate and hydrogens[] to relative coordinates.
		for (size_t k = 0; k < num_frames; ++k)
		{
			frame& f = frames[k];
			const vec3 origin = f.heavy_atoms.front().coordinate; // Cannot use vec3& because origin will be udpated.
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				f.heavy_atoms[i].coordinate -= origin;
			}
			const size_t num_hydrogens = f.hydrogens.size();
			for (size_t i = 0; i < num_hydrogens; ++i)
			{
				f.hydrogens[i].coordinate -= origin;
			}
			f.coordinates.resize(num_heavy_atoms);
			f.derivatives.resize(num_heavy_atoms);
			f.energies.resize(num_heavy_atoms);
		}
	}

	vector<size_t> ligand::get_atom_types() const
	{
		vector<size_t> atom_types;
		atom_types.reserve(10); // A ligand typically consists of <= 10 XScore atom types.
		for (size_t k = 0; k < num_frames; ++k)
		{
			const frame& f = frames[k];
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				const size_t t = f.heavy_atoms[i].xs;
				if (find(atom_types.begin(), atom_types.end(), t) == atom_types.end()) atom_types.push_back(t);
			}
		}
		return atom_types;
	}

	bool ligand::evaluate(const conformation& conf, const scoring_function& sf, const box& b, const vector<array3d<fl>>& grid_maps, const fl e_upper_bound, fl& e, fl& f, change& g)
	{
		if (!b.within(conf.position))
			return false;

		// Apply position and orientation to ROOT frame.
		frame& root = frames.front();
		root.coordinates.front() = conf.position;
		root.orientation_q = conf.orientation;
		root.orientation_m = quaternion_to_matrix(conf.orientation);
		for (size_t i = 1; i < root.heavy_atoms.size(); ++i)
		{
			root.coordinates[i] = conf.position + root.orientation_m * root.heavy_atoms[i].coordinate;
			if (!b.within(root.coordinates[i]))
				return false;
		}

		// Apply torsions to BRANCH frames.
		for (size_t k = 1, t = 0; k < num_frames; ++k)
		{
			frame& f = frames[k];
			const frame& pf = frames[f.parent];

			// Update origin.
			f.coordinates.front() = pf.coordinates.front() + pf.orientation_m * f.relative_origin;
			if (!b.within(f.coordinates.front()))
				return false;

			// If the current BRANCH frame does not have an active torsion, skip it.
			if (!f.active) continue;

			// Update orientation.
			BOOST_ASSERT(f.relative_axis.normalized());
			f.axis = pf.orientation_m * f.relative_axis;
			BOOST_ASSERT(f.axis.normalized());
			f.orientation_q = axis_angle_to_quaternion(f.axis, conf.torsions[t++]) * pf.orientation_q;
			BOOST_ASSERT(quaternion_is_normalized(f.orientation_q));
			f.orientation_m = quaternion_to_matrix(f.orientation_q);

			// Update coordinates.
			const vec3& origin = f.coordinates.front();
			for (size_t i = 1; i < f.heavy_atoms.size(); ++i)
			{
				f.coordinates[i] = origin + f.orientation_m * f.heavy_atoms[i].coordinate;
				if (!b.within(f.coordinates[i]))
					return false;
			}
		}

		// Check steric clash between atoms of different frames except for (rotorX, rotorY) pair.
		//for (size_t k1 = num_frames - 1; k1 > 0; --k1)
		//{
		//	const frame& f1 = frames[k1];
		//	const size_t num_heavy_atoms1 = f1.heavy_atoms.size();
		//	for (size_t i1 = 0; i1 < num_heavy_atoms1; ++i1)
		//	{
		//		for (size_t k2 = 0; k2 < k1; ++k2)
		//		{
		//			const frame& f2 = frames[k2];
		//			const size_t num_heavy_atoms2 = f2.heavy_atoms.size();
		//			for (size_t i2 = 0; i2 < num_heavy_atoms2; ++i2)
		//			{
		//				if ((distance_sqr(f1.coordinates[i1], f2.coordinates[i2]) < sqr(f1.heavy_atoms[i1].covalent_radius() + f2.heavy_atoms[i2].covalent_radius())) && (!((k2 == f1.parent) && (i1 == 0) && (i2 == f1.rotorX))))
		//					return false;
		//			}
		//		}
		//	}
		//}

		e = 0;
		for (size_t k = 0; k < num_frames; ++k)
		{
			frame& f = frames[k];
			const size_t num_heavy_atoms = f.heavy_atoms.size();
			for (size_t i = 0; i < num_heavy_atoms; ++i)
			{
				// Retrieve the grid map in need.
				const array3d<fl>& grid_map = grid_maps[f.heavy_atoms[i].xs];
				BOOST_ASSERT(grid_map.initialized());

				// Find the index and fraction of the current coordinates.
				const array<size_t, 3> index = b.grid_index(f.coordinates[i]);

				// Assert the validity of index.
				BOOST_ASSERT(index[0] < b.num_grids[0]);				
				BOOST_ASSERT(index[1] < b.num_grids[1]);
				BOOST_ASSERT(index[2] < b.num_grids[2]);

				// (x0, y0, z0) is the beginning corner of the partition.
				const size_t x0 = index[0];
				const size_t y0 = index[1];
				const size_t z0 = index[2];
				const fl e000 = grid_map(x0, y0, z0);
				f.energies[i] = e000;

				// The derivative of probe atoms can be precalculated at the cost of massive memory storage.
				const fl e100 = grid_map(x0 + 1, y0,     z0    );
				const fl e010 = grid_map(x0,     y0 + 1, z0    );
				const fl e001 = grid_map(x0,     y0,     z0 + 1);
				f.derivatives[i][0] = (e100 - e000) * b.grid_granularity_inverse;
				f.derivatives[i][1] = (e010 - e000) * b.grid_granularity_inverse;
				f.derivatives[i][2] = (e001 - e000) * b.grid_granularity_inverse;

				e += e000; // Aggregate the energy.
			}
		}

		f = e;

		// Calculate intra-ligand free energy.
		const size_t num_one_to_four_pairs = one_to_four_pairs.size();
		for (size_t i = 0; i < num_one_to_four_pairs; ++i)
		{
			const one_to_four_pair& p = one_to_four_pairs[i];
			frame& f1 = frames[p.k1];
			frame& f2 = frames[p.k2];
			const vec3 r = f2.coordinates[p.i2] - f1.coordinates[p.i1];
			const fl r2 = r.norm_sqr();
			if (r2 < scoring_function::Cutoff_Sqr)
			{
				const scoring_function_element element = sf.evaluate(p.type_pair_index, r2);
				e += element.e;
				const vec3 derivative = element.dor * r;
				f1.derivatives[p.i1] -= derivative;
				f2.derivatives[p.i2] += derivative;
			}
		}

		// If the free energy is no better than the upper bound, refuse this conformation.
		if (e >= e_upper_bound) return false;

		// Initialize force and torque.
		for (size_t k = 0; k < num_frames; ++k)
		{
			frame& f = frames[k];
			f.force  = f.derivatives.front(); // Initialize force to the derivative of origin.
			f.torque = zero3; // Initialize torque to zero.
		}

		// Calculate and aggregate the force and torque of BRANCH frames to their parent frame.
		for (size_t k = num_frames - 1, t = num_active_torsions - 1; k > 0; --k)
		{
			frame&  f = frames[k];
			frame& pf = frames[f.parent];
			const vec3& origin = f.coordinates.front();

			// The origin is ignored for fast evaluation.
			// The force of origin has been counted during initialization.
			// The torque of origin is always zero.
			for (size_t i = 1; i < f.heavy_atoms.size(); ++i)
			{
				// The derivatives with respect to the position, orientation, and torsions
				// would be the negative total force acting on the ligand,
				// the negative total torque, and the negative torque projections, respectively,
				// where the projections refer to the torque applied to the branch moved by the torsion,
				// projected on its rotation axis.
				f.force  += f.derivatives[i];
				f.torque += cross_product(f.coordinates[i] - origin, f.derivatives[i]);
			}

			// Aggregate the force and torque of current frame to its parent frame.
			pf.force  += f.force;
			pf.torque += f.torque + cross_product(origin - pf.coordinates.front(), f.force);

			// If the current BRANCH frame does not have an active torsion, skip it.
			if (!f.active) continue;

			// Save the aggregated torque to torsion.
			g.torsions[t--] = f.torque * f.axis; // dot product
		}

		// Calculate and aggregate the force and torque of ROOT frame.
		const vec3& root_origin = root.coordinates.front();
		for (size_t i = 1; i < root.heavy_atoms.size(); ++i)
		{
			root.force  += root.derivatives[i];
			root.torque += cross_product(root.coordinates[i] - root_origin, root.derivatives[i]);
		}

		// Save the aggregated force and torque to g.position and g.orientation.
		g.position    = root.force;
		g.orientation = root.torque;

		return true;
	}

	result ligand::compose_result(const fl e, const fl f, const conformation& conf) const
	{
		vector<qt> orientations_q(num_frames);
		vector<mat3> orientations_m(num_frames);
		vector<vector<vec3>> heavy_atoms(num_frames);
		vector<vector<vec3>> hydrogens(num_frames);

		// Calculate the coordinates of both heavy atoms and hydrogens of ROOT frame.
		const frame& root = frames.front();
		heavy_atoms.front().resize(root.heavy_atoms.size());
		hydrogens.front().resize(root.hydrogens.size());

		heavy_atoms.front().front() = conf.position;
		orientations_q.front() = conf.orientation;
		orientations_m.front() = quaternion_to_matrix(conf.orientation);
		for (size_t i = 1; i < root.heavy_atoms.size(); ++i)
		{
			heavy_atoms.front()[i] = conf.position + orientations_m.front() * root.heavy_atoms[i].coordinate;
		}
		for (size_t i = 0; i < root.hydrogens.size(); ++i)
		{
			hydrogens.front()[i] = conf.position + orientations_m.front() * root.hydrogens[i].coordinate;
		}

		// Calculate the coordinates of both heavy atoms and hydrogens of BRANCH frames.
		for (size_t k = 1, t = 0; k < num_frames; ++k)
		{
			const frame& f = frames[k];
			heavy_atoms[k].resize(f.heavy_atoms.size());
			hydrogens[k].resize(f.hydrogens.size());

			// Update origin.
			heavy_atoms[k].front() = heavy_atoms[f.parent].front() + orientations_m[f.parent] * f.relative_origin;

			// Update orientation.
			orientations_q[k] = axis_angle_to_quaternion(orientations_m[f.parent] * f.relative_axis, f.active ? conf.torsions[t++] : 0) * orientations_q[f.parent];
			orientations_m[k] = quaternion_to_matrix(orientations_q[k]);

			// Update coordinates.
			const vec3& origin = heavy_atoms[k].front();
			for (size_t i = 1; i < f.heavy_atoms.size(); ++i)
			{
				heavy_atoms[k][i] = origin + orientations_m[k] * f.heavy_atoms[i].coordinate;
			}
			for (size_t i = 0; i < f.hydrogens.size(); ++i)
			{
				hydrogens[k][i] = origin + orientations_m[k] * f.hydrogens[i].coordinate;
			}
		}

		return result(e, f, static_cast<vector<vector<vec3>>&&>(heavy_atoms), static_cast<vector<vector<vec3>>&&>(hydrogens));
	}

	void ligand::write_models(const path& output_ligand, const ptr_vector<result>& results, const size_t num_conformations)
	{
		BOOST_ASSERT(num_conformations > 0);
		BOOST_ASSERT(num_conformations <= results.size());

		const size_t num_lines = lines.size();

		// Dump binding conformations to the output ligand file.
		using namespace std;
		ofstream out(output_ligand); // Dumping starts. Open the file stream as late as possible.
		out.setf(ios::fixed, ios::floatfield);
		out << setprecision(3);
		for (size_t i = 0; i < num_conformations; ++i)
		{
			const result& r = results[i];
			out << "MODEL     " << setw(4) << (i + 1) << '\n'
				<< "REMARK     FREE ENERGY PREDICTED BY IDOCK:" << setw(8) << r.e << " KCAL/MOL\n";
			for (size_t j = 0, frame = 0, heavy_atom = 0, hydrogen = 0; j < num_lines; ++j)
			{
				const string& line = lines[j];
				if (line.size() >= 79) // This line starts with "ATOM" or "HETATM"
				{
					const vec3& coordinate = line[77] == 'H' ? r.hydrogens[frame][hydrogen++] : r.heavy_atoms[frame][heavy_atom++];
					out << line.substr(0, 30)
						<< setw(8) << coordinate[0]
						<< setw(8) << coordinate[1]
						<< setw(8) << coordinate[2]
						<< line.substr(54);
				}
				else // This line starts with "ROOT", "ENDROOT", "BRANCH", "ENDBRANCH", TORSDOF", which will not change during docking.
				{
					out << line;
					if (starts_with(line, "BRANCH"))
					{
						++frame;
						heavy_atom = 0;
						hydrogen = 0;
					}
				}
				out << '\n';
			}
			out << "ENDMDL\n";
		}
		out.close(); // Dumping finishes. Close the file stream as soon as possible.
	}
}
