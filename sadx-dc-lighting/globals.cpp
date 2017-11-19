#include "stdafx.h"

#include <string>
#include <unordered_map>
#include <deque>
#include <ninja.h>
#include "lantern.h"

namespace globals
{
#ifdef _DEBUG
	NJS_VECTOR light_dir = {};
#endif

	HelperFunctions helper_functions {};

	std::unordered_map<NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks {};

	Sint32 light_type       = 0;
	bool landtable_specular = false;
	bool object_vcolor      = true;
	bool first_material     = false;

	std::string mod_path;
	std::string cache_path;
	std::string shader_path;
	LanternCollection palettes = {};
}

std::string globals::get_system_path(const char* path)
{
	std::string result("SYSTEM\\");
	result.append(path);
	result = helper_functions.GetReplaceablePath(result.c_str());
	return result;
}

std::string globals::get_system_path(const std::string& path)
{
	return move(get_system_path(path.c_str()));
}