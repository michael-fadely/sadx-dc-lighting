#include "stdafx.h"
#include <d3d9.h>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Standard library
#include <string>
#include <vector>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "fog.h"
#include "globals.h"
#include "lantern.h"

using namespace std;

enum RenderFlags
{
	EnvironmentMap   = 0x1,
	ConstantMaterial = 0x2,
	OffsetMaterial   = 0x4,
	RenderFlags_8    = 0x8,
	RenderFlags_10   = 0x10,
};

static Trampoline* CharSel_LoadA_t          = nullptr;
static Trampoline* Direct3D_ParseMaterial_t = nullptr;
static Trampoline* GoToNextLevel_t          = nullptr;
static Trampoline* IncrementAct_t           = nullptr;
static Trampoline* LoadLevelFiles_t         = nullptr;
static Trampoline* SetLevelAndAct_t         = nullptr;
static Trampoline* SetTimeOfDay_t           = nullptr;

DataArray(PaletteLight, LightPaletteData, 0x00903E88, 256);
DataArray(StageLightData, CurrentStageLights, 0x03ABD9F8, 4);

DataPointer(EntityData1*, Camera_Data1, 0x03B2CBB0);
DataPointer(NJS_COLOR, EntityVertexColor, 0x03D0848C);
DataPointer(NJS_COLOR, LandTableVertexColor, 0x03D08494);
DataPointer(PaletteLight, LSPalette, 0x03ABDAF0);
DataPointer(Uint32, LastRenderFlags, 0x03D08498);

static void DisplayLightDirection()
{
#ifdef _DEBUG
	using namespace globals;

	auto player = CharObj1Ptrs[0];
	if (player == nullptr)
		return;

	NJS_POINT3 points[2] = {
		player->Position,
		{}
	};

	int colors[2] = {};

	LineInfo info = {
		points, colors, 0, 2
	};

	points[1] = points[0];
	points[1].x += light_dir.x * 10.0f;
	colors[0] = colors[1] = 0xFFFF0000;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].y += light_dir.y * 10.0f;
	colors[0] = colors[1] = 0xFF00FF00;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].z += light_dir.z * 10.0f;
	colors[0] = colors[1] = 0xFF0000FF;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].x += light_dir.x * 10.0f;
	points[1].y += light_dir.y * 10.0f;
	points[1].z += light_dir.z * 10.0f;
	colors[0] = colors[1] = 0xFFFFFF00;
	DrawLineList(&info, 1, 0);
#endif
}

static void SetMaterialParameters(const D3DMATERIAL9& material)
{
	using namespace d3d;

	if (!UsePalette() || effect == nullptr)
		return;

	// This will need to be re-evaluated for chunk models.
	//D3DMATERIALCOLORSOURCE colorsource;
	//device->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, (DWORD*)&colorsource);
	//effect->SetInt("DiffuseSource", colorsource);
	effect->SetVector("MaterialDiffuse", (D3DXVECTOR4*)&material.Diffuse);
}

static void __cdecl CorrectMaterial_r()
{
	using namespace d3d;
	D3DMATERIAL9 material; // [sp+8h] [bp-44h]@1

	device->GetMaterial(&material);
	material.Power = LSPalette.SP_pow;

	material.Ambient.r  /= 255.0f;
	material.Ambient.g  /= 255.0f;
	material.Ambient.b  /= 255.0f;
	material.Ambient.a  /= 255.0f;
	material.Diffuse.r  /= 255.0f;
	material.Diffuse.g  /= 255.0f;
	material.Diffuse.b  /= 255.0f;
	material.Diffuse.a  /= 255.0f;
	material.Specular.r /= 255.0f;
	material.Specular.g /= 255.0f;
	material.Specular.b /= 255.0f;
	material.Specular.a /= 255.0f;

	SetMaterialParameters(material);
	device->SetMaterial(&material);
}

static void __fastcall Direct3D_ParseMaterial_r(NJS_MATERIAL* material)
{
	using namespace d3d;

	auto original = (decltype(Direct3D_ParseMaterial_r)*)Direct3D_ParseMaterial_t->Target();
	original(material);

	if (effect == nullptr)
		return;

	do_effect = false;

	auto pad = ControllerPointers[0];
	if (!UsePalette() || pad && pad->HeldButtons & Buttons_Z)
		return;

	auto flags = material->attrflags;
	if (_nj_control_3d & NJD_CONTROL_3D_CONSTANT_ATTR)
	{
		flags = _nj_constant_or_attr | _nj_constant_and_attr & flags;
	}

	globals::light = (flags & NJD_FLAG_IGNORE_LIGHT) == 0;
	effect->SetBool("AlphaEnabled", (flags & NJD_FLAG_USE_ALPHA) != 0);

	bool use_texture = (flags & NJD_FLAG_USE_TEXTURE) != 0;

	if (use_texture)
	{
		auto texid = material->attr_texId & 0x3FFF;
		auto textures = Direct3D_CurrentTexList->textures;
		NJS_TEXMEMLIST* texmem = textures ? (NJS_TEXMEMLIST*)textures[texid].texaddr : nullptr;
		if (texmem != nullptr)
		{
			auto texture = (Direct3DTexture8*)texmem->texinfo.texsurface.pSurface;
			if (texture != nullptr)
			{
				effect->SetTexture("BaseTexture", texture->GetProxyInterface());
			}
		}
	}

	effect->SetBool("TextureEnabled", use_texture);
	effect->SetBool("EnvironmentMapped", (LastRenderFlags & EnvironmentMap) != 0);

	D3DMATERIAL9 mat;
	device->GetMaterial(&mat);
	SetMaterialParameters(mat);

	do_effect = true;
}

static void __cdecl CharSel_LoadA_r()
{
	auto original = (decltype(CharSel_LoadA_r)*)CharSel_LoadA_t->Target();

	NJS_VECTOR dir = { 1.0f, -1.0f, -1.0f };

	njUnitVector(&dir);
	UpdateLightDirections(dir);

	LoadLanternPalette(LevelIDs_SkyDeck, 0);

	globals::last_level = CurrentLevel;
	globals::last_act = CurrentAct;

	original();
}

static void __cdecl SetLevelAndAct_r(Uint8 level, Uint8 act)
{
	auto original = (decltype(SetLevelAndAct_r)*)SetLevelAndAct_t->Target();
	original(level, act);
	LoadLanternFiles();
}

static void __cdecl GoToNextLevel_r()
{
	auto original = (decltype(GoToNextLevel_r)*)GoToNextLevel_t->Target();
	original();
	LoadLanternFiles();
}

static void __cdecl IncrementAct_r(int amount)
{
	auto original = (decltype(IncrementAct_r)*)IncrementAct_t->Target();
	original(amount);

	if (amount != 0)
	{
		LoadLanternFiles();
	}
}

static void __cdecl SetTimeOfDay_r(Sint8 time)
{
	auto original = (decltype(SetTimeOfDay_r)*)SetTimeOfDay_t->Target();
	original(time);
	LoadLanternFiles();
}

static void __cdecl LoadLevelFiles_r()
{
	auto original = (decltype(LoadLevelFiles_r)*)LoadLevelFiles_t->Target();
	original();
	LoadLanternFiles();
}

extern "C"
{
	EXPORT ModInfo SADXModInfo = { ModLoaderVer };
	EXPORT void __cdecl Init(const char *path)
	{
		auto handle = GetModuleHandle(L"d3d9.dll");
		if (handle == nullptr)
		{
			MessageBoxA(WindowHandle, "Unable to detect Direct3D 9 DLL. The mod will not function.",
				"D3D9 not loaded", MB_OK | MB_ICONERROR);
			return;
		}

		globals::system = path;
		globals::system.append("\\system\\");

		d3d::InitTrampolines();
		CharSel_LoadA_t          = new Trampoline(0x00512BC0, 0x00512BC6, CharSel_LoadA_r);
		Direct3D_ParseMaterial_t = new Trampoline(0x00784850, 0x00784858, Direct3D_ParseMaterial_r);
		GoToNextLevel_t          = new Trampoline(0x00414610, 0x00414616, GoToNextLevel_r);
		IncrementAct_t           = new Trampoline(0x004146E0, 0x004146E5, IncrementAct_r);
		LoadLevelFiles_t         = new Trampoline(0x00422AD0, 0x00422AD8, LoadLevelFiles_r);
		SetLevelAndAct_t         = new Trampoline(0x00414570, 0x00414576, SetLevelAndAct_r);
		SetTimeOfDay_t           = new Trampoline(0x00412C00, 0x00412C05, SetTimeOfDay_r);

		// Correcting a function call since they're relative
		WriteCall(IncrementAct_t->Target(), (void*)0x00424830);

		WriteJump((void*)0x0040A340, CorrectMaterial_r);
	}

	EXPORT void __cdecl OnFrame()
	{
		auto pad = ControllerPointers[0];
		if (pad && pad->PressedButtons & Buttons_C)
		{
			d3d::LoadShader();
			SetFogParameters();
		}

		if (d3d::effect == nullptr)
			return;

		DisplayLightDirection();
	}

	EXPORT void __cdecl OnRenderDeviceLost()
	{
		if (d3d::effect != nullptr)
			d3d::effect->OnLostDevice();
	}

	EXPORT void __cdecl OnRenderDeviceReset()
	{
		if (d3d::effect != nullptr)
		{
			d3d::effect->OnResetDevice();
			SetFogParameters();
		}
	}
}
