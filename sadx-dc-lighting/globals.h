#pragma once

#include <string>
#include <ninja.h>
#include "lantern.h"

namespace globals
{
#ifdef _DEBUG
	extern NJS_VECTOR light_dir;
#endif

	extern Sint32 light_type;
	extern bool no_specular;
	extern bool landtable_specular;
	extern bool object_vcolor;

	extern std::string system;
	extern LanternCollection palettes;
}
