#include "stdafx.h"

#include <Windows.h>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Local
#include "d3d.h"
#include "globals.h"
#include "lantern.h"
#include "../include/lanternapi.h"
#include "Obj_Past.h"
#include "Obj_SkyDeck.h"
#include "Obj_Chaos7.h"
#include "FixChaoGardenMaterials.h"
#include "FixCharacterMaterials.h"
#include "polybuff.h"
#include "apiconfig.h"
#include "stupidbullshit.h"

static Trampoline* CharSel_LoadA_t                 = nullptr;
static Trampoline* Direct3D_ParseMaterial_t        = nullptr;
static Trampoline* GoToNextLevel_t                 = nullptr;
static Trampoline* IncrementAct_t                  = nullptr;
static Trampoline* LoadLevelFiles_t                = nullptr;
static Trampoline* SetLevelAndAct_t                = nullptr;
static Trampoline* GoToNextChaoStage_t             = nullptr;
static Trampoline* SetTimeOfDay_t                  = nullptr;
static Trampoline* DrawLandTable_t                 = nullptr;
static Trampoline* Direct3D_SetTexList_t           = nullptr;
static Trampoline* SetCurrentStageLights_t         = nullptr;
static Trampoline* SetCurrentStageLight_EggViper_t = nullptr;

DataPointer(NJS_VECTOR, NormalScaleMultiplier, 0x03B121F8);

#ifdef DEBUG
static void show_light_direction()
{
	using namespace globals;

	auto player = EntityData1Ptrs[0];

	if (player == nullptr)
	{
		return;
	}

	NJS_POINT3 points[2] = {
		player->Position,
		{}
	};

	NJS_COLOR colors[2] = {};

	NJS_POINT3COL info = {
		points, colors, nullptr, 2
	};

	points[1] = points[0];
	points[1].x += debug_stage_light_dir.x * 10.0f;
	colors[0].color = colors[1].color = 0xFFFF0000;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].y += debug_stage_light_dir.y * 10.0f;
	colors[0].color = colors[1].color = 0xFF00FF00;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].z += debug_stage_light_dir.z * 10.0f;
	colors[0].color = colors[1].color = 0xFF0000FF;
	DrawLineList(&info, 1, 0);

	points[1] = points[0];
	points[1].x += debug_stage_light_dir.x * 10.0f;
	points[1].y += debug_stage_light_dir.y * 10.0f;
	points[1].z += debug_stage_light_dir.z * 10.0f;
	colors[0].color = colors[1].color = 0xFFFFFF00;
	DrawLineList(&info, 1, 0);
}
#endif

static void __cdecl CorrectMaterial_r()
{
	using namespace d3d;
	D3DMATERIAL8 material;

	Direct3D_Device->GetMaterial(&material);

	if (!LanternInstance::use_palette())
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

	Direct3D_Device->SetMaterial(&material);
}

inline void fix_default_color(Uint32& color)
{
	// HACK: fixes stupid default material color
	// TODO: toggle? What if someone actually wants this color?
	if ((color & 0xFFFFFF) == 0xB2B2B2)
	{
		color |= 0xFFFFFF;
	}
}

static void __fastcall Direct3D_ParseMaterial_r(NJS_MATERIAL* material)
{
	using namespace d3d;

	TARGET_DYNAMIC(Direct3D_ParseMaterial)(material);

	if (shaders_null())
	{
		return;
	}

	do_effect = false;

	if (!LanternInstance::use_palette())
	{
		return;
	}

	reset_overrides();

#ifdef _DEBUG
	auto pad = ControllerPointers[0];
	if (pad && pad->HeldButtons & Buttons_Z)
	{
		return;
	}
#endif

	Uint32 flags = material->attrflags;

	if (material->specular.argb.a == 0)
	{
		flags |= NJD_FLAG_IGNORE_SPECULAR;
	}

	if (globals::first_material)
	{
		constexpr uint32_t FLAG_MASK = NJD_FLAG_IGNORE_SPECULAR;

		_nj_constant_attr_and_ &= ~FLAG_MASK;

		if (!(_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR))
		{
			_nj_constant_attr_or_ = flags & FLAG_MASK;
		}
		else
		{
			_nj_constant_attr_or_ &= ~FLAG_MASK;
			_nj_constant_attr_or_ |= flags & FLAG_MASK;
		}

		globals::first_material = false;
		_nj_control_3d_flag_ |= NJD_CONTROL_3D_CONSTANT_ATTR;
	}

	if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
	{
		flags = _nj_constant_attr_or_ | (_nj_constant_attr_and_ & flags);
	}

	fix_default_color(PolyBuffVertexColor.color);
	fix_default_color(LandTableVertexColor.color);

	globals::palettes.set_palettes(globals::light_type, flags);

	set_flags(LanternShaderFlags_Texture, (flags & NJD_FLAG_USE_TEXTURE) != 0);
	set_flags(LanternShaderFlags_Alpha, (flags & NJD_FLAG_USE_ALPHA) != 0);
	set_flags(LanternShaderFlags_EnvMap, (flags & NJD_FLAG_USE_ENV) != 0);
	set_flags(LanternShaderFlags_Light, (flags & NJD_FLAG_IGNORE_LIGHT) == 0);

	do_effect = true;

	if (apiconfig::material_callbacks.empty())
	{
		return;
	}

	auto it = apiconfig::material_callbacks.find(material);
	if (it == apiconfig::material_callbacks.end())
	{
		return;
	}

	for (auto& cb : it->second)
	{
		if (cb(material, flags))
		{
			break;
		}
	}
}

static void __cdecl CharSel_LoadA_r()
{
	const auto original = TARGET_DYNAMIC(CharSel_LoadA);

	globals::palettes.load_palette(LevelIDs_SkyDeck, 0);
	globals::palettes.set_last_level(CurrentLevel, CurrentAct);

	NJS_VECTOR dir = { 1.0f, -1.0f, -1.0f };
	njUnitVector(&dir);
	globals::palettes.light_direction(dir);

	original();
}

static void __cdecl SetLevelAndAct_r(Uint8 level, Uint8 act)
{
	TARGET_DYNAMIC(SetLevelAndAct)(level, act);
	globals::palettes.load_files();
}

static void __cdecl GoToNextChaoStage_r()
{
	TARGET_DYNAMIC(GoToNextChaoStage)();

	const auto level = CurrentLevel;
	const auto act = CurrentAct;

	switch (GetCurrentChaoStage())
	{
		case SADXChaoStage_StationSquare:
			CurrentLevel = LevelIDs_SSGarden;
			CurrentAct = 0;
			break;

		case SADXChaoStage_EggCarrier:
			CurrentLevel = LevelIDs_ECGarden;
			CurrentAct = 0;
			break;

		case SADXChaoStage_MysticRuins:
			CurrentLevel = LevelIDs_MRGarden;
			CurrentAct = 0;
			break;

		case SADXChaoStage_Race:
			CurrentLevel = LevelIDs_ChaoRace;
			CurrentAct = 1;
			break;

		case SADXChaoStage_RaceEntry:
			CurrentLevel = LevelIDs_ChaoRace;
			CurrentAct = 0;
			break;

		default:
			return;
	}

	globals::palettes.load_files();

	CurrentLevel = level;
	CurrentAct = act;
}

static void __cdecl GoToNextLevel_r()
{
	TARGET_DYNAMIC(GoToNextLevel)();
	globals::palettes.load_files();
}

static void __cdecl IncrementAct_r(int amount)
{
	TARGET_DYNAMIC(IncrementAct)(amount);

	if (amount != 0)
	{
		globals::palettes.load_files();
	}
}

static void __cdecl SetTimeOfDay_r(Sint8 time)
{
	TARGET_DYNAMIC(SetTimeOfDay)(time);
	globals::palettes.load_files();
}

static void __cdecl LoadLevelFiles_r()
{
	TARGET_DYNAMIC(LoadLevelFiles)();
	globals::palettes.load_files();
}

static void __cdecl DrawLandTable_r()
{
	if (apiconfig::landtable_specular)
	{
		TARGET_DYNAMIC(DrawLandTable)();
		return;
	}

	const auto flag = _nj_control_3d_flag_;
	const auto or   = _nj_constant_attr_or_;

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
		param::lantern.allow_vcolor = true;

		if (!globals::light_type)
		{
			if (texlist == CommonTextures)
			{
				globals::palettes.set_palettes(0, 0);
				param::lantern.allow_vcolor = apiconfig::object_vcolor;
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if (LevelObjTexlists[i] != texlist)
					{
						continue;
					}

					globals::palettes.set_palettes(0, 0);
					param::lantern.allow_vcolor = apiconfig::object_vcolor;
					break;
				}
			}
		}
	}

	return TARGET_DYNAMIC(Direct3D_SetTexList)(texlist);
}

static void __cdecl NormalScale_r(float x, float y, float z)
{
	if (x > FLT_EPSILON || y > FLT_EPSILON || z > FLT_EPSILON)
	{
		param::lantern.normal_scale = float3(x, y, z);
	}
	else
	{
		param::lantern.normal_scale = float3(1.0f, 1.0f, 1.0f);
	}
}

static void set_light_direction()
{
	if (globals::palettes.size())
	{
		const auto& dir = globals::palettes.light_direction();
		CurrentStageLights[0].direction = dir;
		CurrentStageLights[1].direction = dir;
		CurrentStageLights[2].direction = dir;
		CurrentStageLights[3].direction = dir;
	}
}

static void __cdecl SetCurrentStageLights_r(int level, int act)
{
	TARGET_DYNAMIC(SetCurrentStageLights)(level, act);
	set_light_direction();
}

static void __cdecl SetCurrentStageLight_EggViper_r(ObjectMaster* a1)
{
	TARGET_DYNAMIC(SetCurrentStageLight_EggViper)(a1);
	set_light_direction();
}

void __cdecl InitLandTableMeshSet_r(NJS_MODEL_SADX* model, NJS_MESHSET_SADX* meshset)
{
	if (meshset->buffer)
	{
		return;
	}

	NJS_VECTOR*   normals   = model->normals;
	NJS_POINT3*   points    = model->points;
	NJS_MATERIAL* materials = model->mats;

	MeshSetBuffer_Allocate(meshset);

	if (!meshset->buffer)
	{
		return;
	}

	if ((meshset->type_matId & 0x3FFF) != 0x3FFF)
	{
		const auto& material = materials[meshset->type_matId & 0x3FFF];
		LandTableVertexColor = material.diffuse;
	}

	fix_default_color(LandTableVertexColor.color);

	int i = 0;
	if (meshset->vertuv)
	{
		i = 1;
	}

	if (normals)
	{
		i += 2;
	}

	VBufferFuncPtr func = MeshSetInitFunctions[i][static_cast<uint32_t>(meshset->type_matId) >> 14u];

	if (func)
	{
		func(meshset, points, normals);
	}
}

extern "C"
{
	EXPORT ModInfo SADXModInfo = { ModLoaderVer, nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 };
	EXPORT void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		auto d3d9_handle = GetModuleHandle(L"d3d9.dll");
		auto d3d11_handle = GetModuleHandle(L"d3d11.dll");

		if (d3d9_handle == nullptr && d3d11_handle == nullptr)
		{
			MessageBoxA(WindowHandle,
			            "SADX Lantern Engine will not function without d3d8to9 or d3d8to11 saved to your Sonic Adventure DX folder. "
			            "Download d3d8.dll from from https://github.com/crosire/d3d8to9 or [OTHER URL]",
			            "Lantern Engine Error: Missing d3d8.dll", MB_OK | MB_ICONERROR);

			return;
		}

		if (helperFunctions.Version < 5)
		{
			MessageBoxA(WindowHandle, "Mod loader out of date. SADX Lantern Engine requires API version 5 or newer.",
			            "Lantern Engine Error: Mod loader out of date", MB_OK | MB_ICONERROR);

			return;
		}

		WriteJump(InitLandTableMeshSet, InitLandTableMeshSet_r);

		LanternInstance base(&param::palette_a);
		globals::palettes.add(base);

		globals::helper_functions = helperFunctions;

		globals::mod_path    = path;
		globals::cache_path  = globals::mod_path + "\\cache\\";
		globals::shader_path = globals::get_system_path("lantern.hlsl");

		const std::string config_path = globals::mod_path + "\\config.ini";
		std::array<char, 255> str {};

		GetPrivateProfileStringA("Enhancements", "RangeFog", "False", str.data(), str.size(), config_path.c_str());

		if (!strcmp(str.data(), "True"))
		{
			d3d::set_flags(LanternShaderFlags_RangeFog, true);
		}

		d3d::init_trampolines();

		CharSel_LoadA_t                 = new Trampoline(0x00512BC0, 0x00512BC6, CharSel_LoadA_r);
		Direct3D_ParseMaterial_t        = new Trampoline(0x00784850, 0x00784858, Direct3D_ParseMaterial_r);
		GoToNextLevel_t                 = new Trampoline(0x00414610, 0x00414616, GoToNextLevel_r);
		IncrementAct_t                  = new Trampoline(0x004146E0, 0x004146E5, IncrementAct_r);
		LoadLevelFiles_t                = new Trampoline(0x00422AD0, 0x00422AD8, LoadLevelFiles_r);
		SetLevelAndAct_t                = new Trampoline(0x00414570, 0x00414576, SetLevelAndAct_r);
		GoToNextChaoStage_t             = new Trampoline(0x00715130, 0x00715135, GoToNextChaoStage_r);
		SetTimeOfDay_t                  = new Trampoline(0x00412C00, 0x00412C05, SetTimeOfDay_r);
		DrawLandTable_t                 = new Trampoline(0x0043A6A0, 0x0043A6A8, DrawLandTable_r);
		Direct3D_SetTexList_t           = new Trampoline(0x0077F3D0, 0x0077F3D8, Direct3D_SetTexList_r);
		SetCurrentStageLights_t         = new Trampoline(0x0040A950, 0x0040A955, SetCurrentStageLights_r);
		SetCurrentStageLight_EggViper_t = new Trampoline(0x0057E560, 0x0057E567, SetCurrentStageLight_EggViper_r);

		// Material callback hijack
		WriteJump(reinterpret_cast<void*>(0x0040A340), CorrectMaterial_r);

		FixCharacterMaterials();
		FixChaoGardenMaterials();
		Past_Init();
		SkyDeck_Init();
		Chaos7_Init();

		// Vertex normal correction for certain objects in
		// Red Mountain and Sky Deck.
		WriteCall(reinterpret_cast<void*>(0x00411EDA), NormalScale_r);
		WriteCall(reinterpret_cast<void*>(0x00411F1D), NormalScale_r);
		WriteCall(reinterpret_cast<void*>(0x00411F44), NormalScale_r);
		WriteCall(reinterpret_cast<void*>(0x00412783), NormalScale_r);

		NormalScaleMultiplier = { 1.0f, 1.0f, 1.0f };

		polybuff::rewrite_init();
	}

//#ifdef _DEBUG
	EXPORT void __cdecl OnFrame()
	{
		auto pad = ControllerPointers[0];
		if (pad)
		{
			auto pressed = pad->PressedButtons;
			if (pressed & Buttons_C)
			{
				d3d::load_shader();
			}

			if (pressed & Buttons_Left)
			{
				globals::palettes.load_palette(globals::get_system_path("diffuse test.bin"));
			}
			else if (pressed & Buttons_Right)
			{
				globals::palettes.load_palette(globals::get_system_path("specular test.bin"));
			}
			else if (pressed & Buttons_Down)
			{
				globals::palettes.load_palette(CurrentLevel, CurrentAct);
			}
		}

		if (d3d::shaders_null())
		{
			return;
		}

	#ifdef _DEBUG
		show_light_direction();
	#endif
	}
//#endif
}
