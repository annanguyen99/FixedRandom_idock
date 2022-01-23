#pragma once
#ifndef FIXED_RANDOM_HPP
#define FIXED_RANDOM_HPP

#include "randomnumgen_numpycompatible.hpp"
#include "array.hpp"

#define M_PI 3.14159265358979323846

class FixedRandom{

public:
	//FixedRandom static create_1_rng(int seed, int num_entities, array<double, 3> c0, array<double, 3> c1) {
	//	return FixedRandom(seed, num_entities, c0, c1);
	//}
	FixedRandom(size_t seed, size_t num_entities, array<double, 3> c0, array<double, 3> c1) :
		rng(seed), num_entities(RandomNumGen_NumpyCompatible_64::convert_to_uint_fast32_t(num_entities)), c0(c0), c1(c1) {
	}
	double u01() {
		return uniformDouble(0, 1);
	}
	double u11() {
		return uniformDouble(-1, 1);
	}
	double upi() {
		return uniformDouble(-M_PI, M_PI);
	}
	double ub0() {
		return uniformDouble(c0[0], c1[0]);
	}
	double ub1() {
		return uniformDouble(c0[1], c1[1]);
	}
	double ub2() {
		return uniformDouble(c0[2], c1[2]);
	}
	int    uen() {
		return rng.random_uint32(0, num_entities);
	}
	double n01() {
		return rng.getDouble_GaussianDist();
	}

private:
	RandomNumGen_NumpyCompatible_64 rng;
	//std::uint_fast32_t seed;
	std::uint_fast32_t num_entities;
	array<double, 3> c0, c1;
	//FixedRandom();

	inline double uniformDouble(double lower, double higher) {
		return (rng.getDouble_UniformDist() * (higher - lower)) + lower;
	}
};


#endif