#include "stdafx.h"

#include <ninja.h>
#include <string>

#include "lantern.h"

namespace globals
{
	Sint32 light_type       = 0;
	bool no_specular        = false;
	bool landtable_specular = false;

#ifdef _DEBUG
	NJS_VECTOR light_dir = {};
#endif

	std::string system = "";
	LanternCollection palettes = {};
}
