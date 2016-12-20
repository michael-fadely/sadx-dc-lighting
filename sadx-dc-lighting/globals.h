#pragma once

#include <string>
#include <ninja.h>
#include "lantern.h"

namespace globals
{
	extern NJS_VECTOR light_dir;
	extern Sint32 light_type;
	extern bool fog;
	extern bool light;
	extern bool no_specular;
	extern std::string system;
	extern LanternCollection palettes;
}
