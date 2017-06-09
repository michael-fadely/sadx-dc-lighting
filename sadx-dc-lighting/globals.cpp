#include "stdafx.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <ninja.h>
#include "lantern.h"

namespace globals
{
#ifdef _DEBUG
	NJS_VECTOR light_dir = {};
#endif

	std::unordered_map<NJS_MATERIAL*, std::vector<lantern_material_cb>> material_callbacks {};

	Sint32 light_type       = 0;
	bool no_specular        = false;
	bool landtable_specular = false;
	bool object_vcolor      = true;

	std::string system = "";
	LanternCollection palettes = {};
}
