#include "stdafx.h"
#include <string.h>
#include <ninja.h>

namespace globals
{
	Sint32 light_type = 0;
	bool fog          = true;
	bool light        = true;
	bool no_specular  = false;

	NJS_VECTOR light_dir = {};
	std::string globals::system = "";
	LanternCollection palettes = {};
}
