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
	extern bool fog;
	extern bool light;
	extern bool no_specular;
	extern std::string system;
	extern LanternCollection palettes;
}
