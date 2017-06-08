#include "stdafx.h"

#include <ninja.h>
#include <string>
#include "lantern.h"

namespace globals
{
#ifdef _DEBUG
	NJS_VECTOR light_dir = {};
#endif

	Sint32 light_type       = 0;
	bool no_specular        = false;
	bool landtable_specular = false;
	bool object_vcolor      = true;

	std::string system = "";
	LanternCollection palettes = {};
}
