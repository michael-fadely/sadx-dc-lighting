#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <ninja.h>
#include "lantern.h"
#include "../include/lanternapi.h"

namespace globals
{
#ifdef _DEBUG
	extern NJS_VECTOR light_dir;
#endif

	extern std::unordered_map<NJS_MATERIAL*, std::vector<lantern_material_cb>> material_callbacks;

	extern Sint32 light_type;
	extern bool no_specular;
	extern bool landtable_specular;
	extern bool object_vcolor;

	extern std::string system;
	extern LanternCollection palettes;
}
