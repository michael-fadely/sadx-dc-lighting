#include "stdafx.h"

// Direct3D
#include <d3d9.h>

// Mod loader
#include <Trampoline.h>

// Local
#include "d3d.h"
#include "globals.h"

static D3DFOGMODE fog_mode = D3DFOG_NONE;

static void __cdecl njDisableFog_r();
static void __cdecl njEnableFog_r();
static void __cdecl njSetFogColor_r(Uint32 c);
static void __cdecl njSetFogTable_r(NJS_FOG_TABLE fogtable);

static Trampoline njDisableFog_t(0x00794980, 0x00794985, njDisableFog_r);
static Trampoline njEnableFog_t(0x00794950, 0x00794955, njEnableFog_r);
static Trampoline njSetFogColor_t(0x00787240, 0x00787245, njSetFogColor_r);
static Trampoline njSetFogTable_t(0x00794E40, 0x00794E46, njSetFogTable_r);

using namespace d3d;

static void __cdecl njDisableFog_r()
{
	TARGET_STATIC(njDisableFog)();

	if (effect == nullptr)
		return;

	if (globals::fog)
	{
		effect->SetInt("FogMode", D3DFOG_NONE);
	}
	globals::fog = false;
}

static void __cdecl njEnableFog_r()
{
	TARGET_STATIC(njEnableFog)();

	if (effect == nullptr)
		return;

	if (!globals::fog)
	{
		effect->SetInt("FogMode", fog_mode);
	}
	globals::fog = true;
}

static void __cdecl njSetFogColor_r(Uint32 c)
{
	TARGET_STATIC(njSetFogColor)(c);

	if (effect == nullptr)
		return;

	auto color = D3DXCOLOR(c);

	static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4),
		"D3DXCOLOR and D3DXVECTOR4 size mismatch in fog setup.");

	effect->SetVector("FogColor", (D3DXVECTOR4*)&color);
}

void SetFogParameters()
{
	if (effect == nullptr)
		return;

	device->GetRenderState(D3DRS_FOGTABLEMODE, (DWORD*)&fog_mode);
	effect->SetInt("FogMode", fog_mode);
	globals::fog = true;

	float start, end, density;
	device->GetRenderState(D3DRS_FOGSTART, (DWORD*)&start);
	device->GetRenderState(D3DRS_FOGEND, (DWORD*)&end);

	effect->SetFloat("FogStart", start);
	effect->SetFloat("FogEnd", end);

	if (fog_mode != D3DFOG_LINEAR)
	{
		device->GetRenderState(D3DRS_FOGDENSITY, (DWORD*)&density);
		effect->SetFloat("FogDensity", density);
	}
}

static void __cdecl njSetFogTable_r(NJS_FOG_TABLE fogtable)
{
	TARGET_STATIC(njSetFogTable)(fogtable);
	SetFogParameters();
}
