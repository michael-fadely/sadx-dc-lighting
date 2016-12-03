#include "stdafx.h"
#include "globals.h"

namespace globals
{
	Sint32 last_type  = 0;
	Sint8 last_time   = 0;
	Uint32 last_act   = 0;
	Uint32 last_level = 0;
	bool fog          = true;
	bool light        = true;

	NJS_VECTOR light_dir = {};
	std::string globals::system = "";
}
