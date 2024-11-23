#pragma once

#include <array>
#include <deque>
#include <unordered_map>

#include <ninja.h>

#include "..\include\lanternapi.h"

enum OverrideFlags : uint8_t
{
	OverrideFlags_None = 0u,
	OverrideFlags_Temporary = 1u << 0,
	OverrideFlags_Permanent = 1u << 1,
};

class apiconfig
{
public:
	static std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks;

	static bool object_vcolor;

	static uint8_t light_dir_override_flags;
	static std::array<NJS_VECTOR, 2> light_dir_overrides;

	static float alpha_ref_value;
	static bool alpha_ref_is_temp;

	static void set_light_direction_override(uint8_t flags, const NJS_VECTOR& v);
	static NJS_VECTOR get_light_direction_override();
};
