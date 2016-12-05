#include "stdafx.h"
#include "globals.h"

namespace globals
{
	Sint32 light_type = 0;
	Sint8 last_time   = 0;
	Uint32 last_act   = 0;
	Uint32 last_level = 0;
	bool fog          = true;
	bool light        = true;
	bool no_specular  = false;

	NJS_VECTOR light_dir = {};
	std::string globals::system = "";
}
