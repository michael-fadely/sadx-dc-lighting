#include "stdafx.h"
#include "floatmath.h"
#include "lanternlight.h"

bool DirLightData_hlsl::operator==(const DirLightData_hlsl& other) const
{
	return equal(direction, other.direction)
	    && equal(color, other.color)
	    && near_equal(specular_m, other.specular_m)
	    && near_equal(diffuse_m, other.diffuse_m)
	    && near_equal(ambient_m, other.ambient_m);
}

bool DirLightData_hlsl::operator!=(const DirLightData_hlsl& other) const
{
	return !operator==(other);
}
