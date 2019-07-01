#pragma once

#include <string>
#include <unordered_map>
#include <deque>
#include <ninja.h>
#include "lantern.h"
#include "../include/lanternapi.h"

#include <SADXModInfo.h>

namespace globals
{
	extern NJS_VECTOR debug_stage_light_dir;

	extern HelperFunctions helper_functions;

	extern Sint32 light_type;
	extern bool first_material;

	extern std::string mod_path;
	extern std::string cache_path;
	extern std::string shader_path;
	extern LanternCollection palettes;

	std::string get_system_path(const char* path);
	std::string get_system_path(const std::string& path);
}
