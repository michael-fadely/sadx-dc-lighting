#include "stdafx.h"
#include "floatmath.h"

#include <cmath>
#include <limits>

bool near_equal(float a, float b)
{
	constexpr auto epsilon = std::numeric_limits<float>::epsilon();
	return std::abs(a - b) <= epsilon;
}

bool equal(const NJS_VECTOR& a, const NJS_VECTOR& b)
{
	return near_equal(a.x, b.x)
		&& near_equal(a.y, b.y)
		&& near_equal(a.z, b.z);
}

bool not_equal(const NJS_VECTOR& a, const NJS_VECTOR& b)
{
	return !near_equal(a.x, b.x)
		|| !near_equal(a.y, b.y)
		|| !near_equal(a.z, b.z);
}
