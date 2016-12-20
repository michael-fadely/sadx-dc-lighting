#include "stdafx.h"
#include <SADXModLoader.h> 
#include "Obj_SkyDeck.h"
#include "globals.h"

static size_t handle = 0;

// I don't think it actually has a delete function, but I need one.
void __cdecl Obj_SkyDeck_Delete_r(ObjectMaster* _this)
{
	globals::palettes.Remove(handle);
	LanternInstance::SetBlendFactor(0.0f);
}

Trampoline Obj_SkyDeck_t(0x005F02E0, 0x005F02E5, Obj_SkyDeck_r);

void __cdecl Obj_SkyDeck_r(ObjectMaster* _this)
{
	TARGET_STATIC(Obj_SkyDeck)(_this);

	if (handle || d3d::effect == nullptr)
	{
		return;
	}

	_this->DeleteSub = Obj_SkyDeck_Delete_r;

	globals::palettes.LoadPalette(LevelIDs_SkyDeck, 0);
	globals::palettes.LoadSource(LevelIDs_SkyDeck, 0);

	LanternInstance asdf(&param::DiffusePaletteB, &param::SpecularPaletteB);
	asdf.LoadPalette(LevelIDs_SkyDeck, 1);
	handle = globals::palettes.Add(asdf);
}
