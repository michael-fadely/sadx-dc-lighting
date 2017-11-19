#pragma once

#include <string>
#include <unordered_map>
#include <deque>
#include <ninja.h>
#include "lantern.h"
#include "../include/lanternapi.h"

namespace globals
{
#ifdef _DEBUG
	extern NJS_VECTOR light_dir;
#endif

	extern HelperFunctions helper_functions;

	std::string get_system_path(const char* path);
	std::string get_system_path(const std::string& path);

	extern std::unordered_map<NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks;

	extern Sint32 light_type;
	extern bool landtable_specular;
	extern bool object_vcolor;
	extern bool first_material;

	extern std::string mod_path;
	extern std::string cache_path;
	extern std::string shader_path;
	extern LanternCollection palettes;
}
