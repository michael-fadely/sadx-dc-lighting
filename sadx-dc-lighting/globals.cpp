#include "stdafx.h"

#include <string>
#include <unordered_map>
#include <deque>
#include <ninja.h>
#include "lantern.h"

#include <SADXModInfo.h>

namespace globals
{
	NJS_VECTOR debug_stage_light_dir = {};

	HelperFunctions helper_functions {};

	// TODO: why is this in "globals"
	std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks {};

	// TODO: move all the API stuff to some specific namespace or singleton

	Sint32 light_type       = 0;
	bool landtable_specular = false;
	bool object_vcolor      = true;
	bool override_light_dir = false;
	bool first_material     = false;

	NJS_VECTOR light_dir_override { 0.0f, -1.0f, 0.0f };

	std::string mod_path;
	std::string cache_path;
	std::string shader_path;
	LanternCollection palettes = {};

	std::string get_system_path(const char* path)
	{
		std::string result("SYSTEM\\");
		result.append(path);
		result = helper_functions.GetReplaceablePath(result.c_str());
		return result;
	}

	std::string get_system_path(const std::string& path)
	{
		return get_system_path(path.c_str());
	}
}
