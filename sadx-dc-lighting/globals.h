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

	// TODO: why is this in "globals"
	extern std::unordered_map<const NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks;

	// TODO: move all the API stuff to some specific namespace or singleton

	extern Sint32 light_type;
	extern bool landtable_specular;
	extern bool object_vcolor;
	extern bool override_light_dir;
	extern bool first_material;

	extern NJS_VECTOR light_dir_override;

	extern std::string mod_path;
	extern std::string cache_path;
	extern std::string shader_path;
	extern LanternCollection palettes;

	std::string get_system_path(const char* path);
	std::string get_system_path(const std::string& path);
}
