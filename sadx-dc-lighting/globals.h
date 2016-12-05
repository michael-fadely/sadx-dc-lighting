#pragma once

#include <string>
#include <ninja.h>

namespace globals
{
	extern NJS_VECTOR light_dir;
	extern Sint32 light_type;
	extern Sint8 last_time;
	extern Uint32 last_act;
	extern Uint32 last_level;
	extern bool fog;
	extern bool light;
	extern bool no_specular;
	extern std::string system;
}
