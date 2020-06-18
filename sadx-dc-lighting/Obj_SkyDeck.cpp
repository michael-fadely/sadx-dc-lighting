#include "stdafx.h"

#include "d3d.h"
#include <SADXModLoader.h> 

#include "Trampoline.h"
#include "globals.h"
#include "../include/lanternapi.h"

#include "Obj_SkyDeck.h"

static Trampoline* SkyDeck_SimulateAltitude_t = nullptr;
static Trampoline* SkyBox_SkyDeck_t = nullptr;
static Trampoline* Obj_SkyDeck_t = nullptr;
static size_t handle = 0;

static void __cdecl SkyDeck_SimulateAltitude_r(Uint16 act)
{
	TARGET_DYNAMIC(SkyDeck_SimulateAltitude)(act);

	// 0 = high altitude (bright), 1 = low altitude (dark)
	if (SkyDeck_AltitudeMode > 1)
	{
		set_blend_factor(0.0f);
	}
	else
	{
		float f = (std::max(180.0f, std::min(250.0f, SkyDeck_SkyAltitude)) - 180.0f) / 70.0f;
		set_blend_factor(f);
	}
}

static void __cdecl SkyBox_SkyDeck_Delete(ObjectMaster*)
{
	// Disable blending in the shader.
	set_shader_flags(ShaderFlags_Blend, false);
	// Reset blend indices.
	set_blend(-1, -1);
}

static void __cdecl SkyBox_SkyDeck_r(ObjectMaster* _this)
{
	if (_this->DeleteSub != SkyBox_SkyDeck_Delete)
	{
		_this->DeleteSub = SkyBox_SkyDeck_Delete;
	}

	TARGET_DYNAMIC(SkyBox_SkyDeck)(_this);
}

static void __cdecl Obj_SkyDeck_Delete(ObjectMaster* _this)
{
	globals::palettes.remove(handle);
	set_shader_flags(ShaderFlags_Blend, false);
	globals::palettes.forward_blend_all(false);
	handle = 0;
}

static void __cdecl Obj_SkyDeck_r(ObjectMaster* _this)
{
	TARGET_DYNAMIC(Obj_SkyDeck)(_this);

	if (handle != 0 || d3d::shaders_null())
	{
		return;
	}

	_this->DeleteSub = Obj_SkyDeck_Delete;

	globals::palettes.load_palette(LevelIDs_SkyDeck, 0);
	globals::palettes.load_source(LevelIDs_SkyDeck, 0);

	param::PaletteB = nullptr;
	LanternInstance lantern(&param::PaletteB);

	lantern.load_palette(LevelIDs_SkyDeck, 1);
	handle = globals::palettes.add(lantern);
	globals::palettes.set_last_level(-1, -1);
	globals::palettes.forward_blend_all(true);
	set_shader_flags(ShaderFlags_Blend, true);
}

void SkyDeck_Init()
{
	Obj_SkyDeck_t              = new Trampoline(0x005F02E0, 0x005F02E5, Obj_SkyDeck_r);
	SkyDeck_SimulateAltitude_t = new Trampoline(0x005ECA80, 0x005ECA87, SkyDeck_SimulateAltitude_r);
	SkyBox_SkyDeck_t           = new Trampoline(0x005F0340, 0x005F0347, SkyBox_SkyDeck_r);
}
