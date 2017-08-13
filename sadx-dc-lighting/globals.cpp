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

	std::unordered_map<NJS_MATERIAL*, std::deque<lantern_material_cb>> material_callbacks {};

	Sint32 light_type       = 0;
	bool landtable_specular = false;
	bool object_vcolor      = true;
	bool first_material     = false;

	std::string system_path;
	std::string cache_path;
	LanternCollection palettes = {};
}
