#include "stdafx.h"
#include "globals.h"

NJS_VECTOR globals::light_dir = {};
Sint32 globals::last_type     = 0;
Sint8 globals::last_time      = 0;
Uint32 globals::last_act      = 0;
Uint32 globals::last_level    = 0;
bool globals::fog             = true;
bool globals::light           = true;
std::string globals::system   = "";

