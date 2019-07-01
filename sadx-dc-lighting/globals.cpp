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

	Sint32 light_type   = 0;
	bool first_material = false;

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
