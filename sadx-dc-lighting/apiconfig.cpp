#include "stdafx.h"
#include "apiconfig.h"

std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> apiconfig::material_callbacks {};

bool apiconfig::object_vcolor = true;

uint8_t apiconfig::light_dir_override_flags = OverrideFlags_None;
std::array<NJS_VECTOR, 2> apiconfig::light_dir_overrides {};

float apiconfig::alpha_ref_value   = 16.0f / 255.0f;
bool  apiconfig::alpha_ref_is_temp = false;

void apiconfig::set_light_direction_override(uint8_t flags, const NJS_VECTOR& v)
{
	if (flags & OverrideFlags_Temporary)
	{
		light_dir_overrides[0] = v;
	}

	if (flags & OverrideFlags_Permanent)
	{
		light_dir_overrides[1] = v;
	}

	light_dir_override_flags |= flags;
}

NJS_VECTOR apiconfig::get_light_direction_override()
{
	if (light_dir_override_flags & OverrideFlags_Temporary)
	{
		return light_dir_overrides[0];
	}

	if (light_dir_override_flags & OverrideFlags_Permanent)
	{
		return light_dir_overrides[1];
	}

	return {};
}
