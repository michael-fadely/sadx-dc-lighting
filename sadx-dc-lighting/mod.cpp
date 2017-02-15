#include "stdafx.h"
#include <d3d9.h>
#include <vector>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"
#include "Obj_Past.h"
#include "Obj_SkyDeck.h"
#include "FixChaoGardenMaterials.h"
#include "FixCharacterMaterials.h"

static Trampoline* CharSel_LoadA_t            = nullptr;
static Trampoline* Direct3D_ParseMaterial_t   = nullptr;
static Trampoline* GoToNextLevel_t            = nullptr;
static Trampoline* IncrementAct_t             = nullptr;
static Trampoline* LoadLevelFiles_t           = nullptr;
static Trampoline* SetLevelAndAct_t           = nullptr;
static Trampoline* GoToNextChaoStage_t        = nullptr;
static Trampoline* SetTimeOfDay_t             = nullptr;
static Trampoline* DrawLandTable_t            = nullptr;
static Trampoline* Direct3D_SetTexList_t      = nullptr;

DataArray(PaletteLight, LightPaletteData, 0x00903E88, 256);
DataArray(StageLightData, CurrentStageLights, 0x03ABD9F8, 4);
DataArray(NJS_TEXLIST*, LevelObjTexlists, 0x03B290B4, 4);

DataPointer(EntityData1*, Camera_Data1, 0x03B2CBB0);
DataPointer(NJS_COLOR, EntityVertexColor, 0x03D0848C);
DataPointer(NJS_COLOR, LandTableVertexColor, 0x03D08494);
DataPointer(PaletteLight, LSPalette, 0x03ABDAF0);
DataPointer(Uint32, LastRenderFlags, 0x03D08498);
DataPointer(NJS_VECTOR, NormalScaleMultiplier, 0x03B121F8);
DataPointer(NJS_TEXLIST*, CommonTextures, 0x03B290B0);

#ifdef _DEBUG
static void DisplayLightDirection()
{
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
}
#endif

static void SetMaterialParameters(const D3DMATERIAL9& material)
{
	using namespace d3d;

	if (!LanternInstance::UsePalette() || effect == nullptr)
		return;

	D3DMATERIALCOLORSOURCE colorsource;
	device->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, (DWORD*)&colorsource);

	param::DiffuseSource   = colorsource;
	param::MaterialDiffuse = material.Diffuse;

#ifdef USE_SL
	param::MaterialSpecular = material.Specular;
	param::MaterialPower    = material.Power;
#endif
}

static void __cdecl CorrectMaterial_r()
{
	using namespace d3d;
	D3DMATERIAL9 material;

	device->GetMaterial(&material);

	if (!LanternInstance::UsePalette())
	{
		material.Power = LSPalette.SP_pow;
	}

	material.Ambient.r /= 255.0f;
	material.Ambient.g /= 255.0f;
	material.Ambient.b /= 255.0f;
	material.Ambient.a /= 255.0f;
	material.Diffuse.r /= 255.0f;
	material.Diffuse.g /= 255.0f;
	material.Diffuse.b /= 255.0f;
	material.Diffuse.a /= 255.0f;
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

	TARGET_DYNAMIC(Direct3D_ParseMaterial)(material);

	if (effect == nullptr)
	{
		return;
	}

	do_effect = false;

	if (!LanternInstance::UsePalette())
	{
		return;
	}

#ifdef _DEBUG
	auto pad = ControllerPointers[0];
	if (pad && pad->HeldButtons & Buttons_Z)
	{
		return;
	}
#endif

	Uint32 flags = material->attrflags;
	Uint32 texid = material->attr_texId & 0xFFFF;

	if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
	{
		flags = _nj_constant_attr_or_ | _nj_constant_attr_and_ & flags;
	}

	globals::palettes.SetPalettes(globals::light_type, globals::no_specular ? flags | NJD_FLAG_IGNORE_SPECULAR : flags);

	bool use_texture = (flags & NJD_FLAG_USE_TEXTURE) != 0;
	SetShaderOptions(ShaderOptions::UseTexture, use_texture);
	SetShaderOptions(ShaderOptions::UseAlpha, (flags & NJD_FLAG_USE_ALPHA) != 0);
	SetShaderOptions(ShaderOptions::UseEnvMap, (flags & NJD_FLAG_USE_ENV) != 0);
	SetShaderOptions(ShaderOptions::UseLight, (flags & NJD_FLAG_IGNORE_LIGHT) == 0);

	// Environment map matrix
	param::TextureTransform = *(D3DXMATRIX*)0x038A5DD0;
	
	if (use_texture)
	{
		auto textures = Direct3D_CurrentTexList->textures;
		NJS_TEXMEMLIST* texmem = textures ? (NJS_TEXMEMLIST*)textures[texid].texaddr : nullptr;
		if (texmem != nullptr)
		{
			auto texture = (Direct3DTexture8*)texmem->texinfo.texsurface.pSurface;
			if (texture != nullptr)
			{
				param::BaseTexture = texture->GetProxyInterface();
			}
		}
	}

	D3DMATERIAL9 mat;
	device->GetMaterial(&mat);
	SetMaterialParameters(mat);

	do_effect = true;
}

static void __cdecl CharSel_LoadA_r()
{
	auto original = TARGET_DYNAMIC(CharSel_LoadA);

	NJS_VECTOR dir = { 1.0f, -1.0f, -1.0f };

	njUnitVector(&dir);
	UpdateLightDirections(dir);

	globals::palettes.LoadPalette(LevelIDs_SkyDeck, 0);
	globals::palettes.SetLastLevel(CurrentLevel, CurrentAct);

	original();
}

static void __cdecl SetLevelAndAct_r(Uint8 level, Uint8 act)
{
	TARGET_DYNAMIC(SetLevelAndAct)(level, act);
	globals::palettes.LoadFiles();
}

static void __cdecl GoToNextChaoStage_r()
{
	TARGET_DYNAMIC(GoToNextChaoStage)();
	switch (GetCurrentChaoStage())
	{
		case SADXChaoStage_StationSquare:
			CurrentLevel = LevelIDs_SSGarden;
			break;

		case SADXChaoStage_EggCarrier:
			CurrentLevel = LevelIDs_ECGarden;
			break;

		case SADXChaoStage_MysticRuins:
			CurrentLevel = LevelIDs_MRGarden;
			break;

		default:
			return;
	}

	globals::palettes.LoadFiles();
}

static void __cdecl GoToNextLevel_r()
{
	TARGET_DYNAMIC(GoToNextLevel)();
	globals::palettes.LoadFiles();
}

static void __cdecl IncrementAct_r(int amount)
{
	TARGET_DYNAMIC(IncrementAct)(amount);

	if (amount != 0)
	{
		globals::palettes.LoadFiles();
	}
}

static void __cdecl SetTimeOfDay_r(Sint8 time)
{
	TARGET_DYNAMIC(SetTimeOfDay)(time);
	globals::palettes.LoadFiles();
}

static void __cdecl LoadLevelFiles_r()
{
	TARGET_DYNAMIC(LoadLevelFiles)();
	globals::palettes.LoadFiles();
}

static void __cdecl DrawLandTable_r()
{
	auto flag = _nj_control_3d_flag_;
	auto or = _nj_constant_attr_or_;

	_nj_control_3d_flag_ |= NJD_CONTROL_3D_CONSTANT_ATTR;
	_nj_constant_attr_or_ |= NJD_FLAG_IGNORE_SPECULAR;

	TARGET_DYNAMIC(DrawLandTable)();

	_nj_control_3d_flag_ = flag;
	_nj_constant_attr_or_ = or;
}

static Sint32 __fastcall Direct3D_SetTexList_r(NJS_TEXLIST* texlist)
{
	if (texlist != Direct3D_CurrentTexList)
	{
		globals::no_specular = false;

		if (!globals::light_type)
		{
			if (texlist == CommonTextures)
			{
				globals::palettes.SetPalettes(0, 0);
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if (LevelObjTexlists[i] != texlist)
						continue;

					globals::palettes.SetPalettes(0, NJD_FLAG_IGNORE_SPECULAR);
					globals::no_specular = true;
					break;
				}
			}
		}
	}

	return TARGET_DYNAMIC(Direct3D_SetTexList)(texlist);
}

static void __cdecl NormalScale(float x, float y, float z)
{
	if (x > FLT_EPSILON || y > FLT_EPSILON || z > FLT_EPSILON)
	{
		param::NormalScale = D3DXVECTOR3(x, y, z);
	}
	else
	{
		param::NormalScale = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	}
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

		LanternInstance base(&param::PaletteA, &param::DiffuseIndexA, &param::SpecularIndexA);
		globals::palettes.Add(base);

		globals::system = path;
		globals::system.append("\\system\\");

		d3d::InitTrampolines();
		CharSel_LoadA_t          = new Trampoline(0x00512BC0, 0x00512BC6, CharSel_LoadA_r);
		Direct3D_ParseMaterial_t = new Trampoline(0x00784850, 0x00784858, Direct3D_ParseMaterial_r);
		GoToNextLevel_t          = new Trampoline(0x00414610, 0x00414616, GoToNextLevel_r);
		IncrementAct_t           = new Trampoline(0x004146E0, 0x004146E5, IncrementAct_r);
		LoadLevelFiles_t         = new Trampoline(0x00422AD0, 0x00422AD8, LoadLevelFiles_r);
		SetLevelAndAct_t         = new Trampoline(0x00414570, 0x00414576, SetLevelAndAct_r);
		GoToNextChaoStage_t      = new Trampoline(0x00715130, 0x00715135, GoToNextChaoStage_r);
		SetTimeOfDay_t           = new Trampoline(0x00412C00, 0x00412C05, SetTimeOfDay_r);
		DrawLandTable_t          = new Trampoline(0x0043A6A0, 0x0043A6A8, DrawLandTable_r);
		Direct3D_SetTexList_t    = new Trampoline(0x0077F3D0, 0x0077F3D8, Direct3D_SetTexList_r);

		// Correcting a function call since they're relative
		WriteCall(IncrementAct_t->Target(), (void*)0x00424830);

		// Material callback hijack
		WriteJump((void*)0x0040A340, CorrectMaterial_r);

		FixCharacterMaterials();
		FixChaoGardenMaterials();
		Past_Init();
		SkyDeck_Init();

		// Vertex normal correction for certain objects in
		// Red Mountain and Sky Deck.
		WriteCall((void*)0x00411EDA, NormalScale);
		WriteCall((void*)0x00411F1D, NormalScale);
		WriteCall((void*)0x00411F44, NormalScale);
		WriteCall((void*)0x00412783, NormalScale);

		NormalScaleMultiplier = { 1.0f, 1.0f, 1.0f };
	}

#ifdef _DEBUG
	EXPORT void __cdecl OnFrame()
	{
		auto pad = ControllerPointers[0];
		if (pad)
		{
			auto pressed = pad->PressedButtons;
			if (pressed & Buttons_C)
			{
				d3d::LoadShader();
			}

			if (pressed & Buttons_Left)
			{
				globals::palettes.LoadPalette(globals::system + "diffuse test.bin");
			}
			else if (pressed & Buttons_Right)
			{
				globals::palettes.LoadPalette(globals::system + "specular test.bin");
			}
			else if (pressed & Buttons_Down)
			{
				globals::palettes.LoadPalette(CurrentLevel, CurrentAct);
			}
		}

		if (d3d::effect == nullptr)
		{
			return;
		}

		DisplayLightDirection();
	}
#endif
}
