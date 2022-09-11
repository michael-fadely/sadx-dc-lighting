#include "stdafx.h"
#include "d3d8types.hpp"
#include <SADXModLoader.h>
#include "FixLightTypes.h"

void SetTexlist_ForceBossLightType(NJS_TEXLIST* tl)
{
	njSetTexture(tl);
	___dsSetPalette(4);
}

void SetTexlist_ForceDefaultLightType(NJS_TEXLIST* tl)
{
	njSetTexture(tl);
	___dsSetPalette(0);
}

void SetTexlist_ForceCharacterLightType(NJS_TEXLIST* tl)
{
	njSetTexture(tl);
	___dsSetPalette(2);
}

void FixLightTypes()
{
	// Chaos 0 Event
	WriteCall(reinterpret_cast<void*>(0x006EDE15), SetTexlist_ForceCharacterLightType);
	// Chaos 1
	WriteCall(reinterpret_cast<void*>(0x006F5D28), SetTexlist_ForceCharacterLightType);
	WriteCall(reinterpret_cast<void*>(0x006F5D6A), SetTexlist_ForceCharacterLightType);
	WriteCall(reinterpret_cast<void*>(0x006F5DDE), SetTexlist_ForceCharacterLightType);
	WriteCall(reinterpret_cast<void*>(0x006F5E45), SetTexlist_ForceCharacterLightType);
	// Tornado
	WriteData<1>(reinterpret_cast<char*>(0x0062751B), 0i8); // Force Tornado light type
	WriteData<1>(reinterpret_cast<char*>(0x0062AC1F), 0i8); // Force Tornado light type (transformation cutscene)
	// Gamma
	WriteData<1>(reinterpret_cast<char*>(0x0047FDB9), 0x02u); // Force character light type
	WriteData<1>(reinterpret_cast<char*>(0x00480078), 0x02u); // Force character light type
	WriteData<1>(reinterpret_cast<char*>(0x00480080), 0x00u); // Force default light type for Jet Booster
	// Question Mark
	WriteCall(reinterpret_cast<void*>(0x0051261E), SetTexlist_ForceDefaultLightType);
	// Chaos 2
	WriteCall(reinterpret_cast<void*>(0x0054D890), SetTexlist_ForceBossLightType);
	WriteCall(reinterpret_cast<void*>(0x0054EAC7), SetTexlist_ForceBossLightType);
	WriteCall(reinterpret_cast<void*>(0x0054F442), SetTexlist_ForceBossLightType);
	WriteCall(reinterpret_cast<void*>(0x0054F51E), SetTexlist_ForceBossLightType);
	// Chaos 6
	WriteCall(reinterpret_cast<void*>(0x00558E6B), SetTexlist_ForceBossLightType);
	// Perfect Chaos
	WriteData<1>(reinterpret_cast<char*>(0x0056220C), 0x04u); // Force boss light type
	// Super Sonic
	WriteData<1>(reinterpret_cast<char*>(0x00494935), 0x02u); // Force character light type
	// Birdie
	WriteData<1>(reinterpret_cast<char*>(0x004C6335), 0i8); // Force default light type
}