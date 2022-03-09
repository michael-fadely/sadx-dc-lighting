#pragma once

#include <deque>
#include <unordered_map>

#include <ninja.h>

#include "..\include\lanternapi.h"

class apiconfig
{
public:
	static std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks;

	static bool object_vcolor;
	static bool object_mcolor;
	static bool override_light_dir;

	static NJS_VECTOR light_dir_override;

	static float alpha_ref_value;
	static bool alpha_ref_is_temp;
};
