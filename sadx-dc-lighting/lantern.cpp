#include "stdafx.h"

#include <d3d9.h>
#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <SADXModLoader.h>

#include "d3d.h"
#include "lantern.h"
#include "globals.h"
#include "datapointers.h"

#pragma pack(push, 1)
struct ColorPair
{
	NJS_COLOR diffuse, specular;
};
#pragma pack(pop)

static_assert(sizeof(ColorPair) == sizeof(NJS_COLOR) * 2, "AGAIN TRY");

/// <summary>
/// Get the pixel data of the specified texture.
/// </summary>
/// <param name="texture">The texture to lock.</param>
/// <returns>The locked rect.</returns>
inline D3DLOCKED_RECT GetTextureRect(IDirect3DTexture9* texture)
{
	D3DLOCKED_RECT result;
	HRESULT hresult = texture->LockRect(0, &result, nullptr, 0);
	if (FAILED(hresult))
	{
		throw std::exception("Failed to lock texture rect!");
	}
	return result;
}

/// <summary>
/// Creates a 1x256 image to be used as a palette.
/// </summary>
/// <param name="texture">Destination texture.</param>
inline void CreateTexture(IDirect3DTexture9*& texture)
{
	if (texture)
	{
		texture->Release();
		texture = nullptr;
	}

	if (FAILED(d3d::device->CreateTexture(256, 1, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
	{
		throw std::exception("Failed to create palette texture!");
	}
}

/// <summary>
/// Creates a pair of texture palettes from the provided color pairs.
/// </summary>
/// <param name="palette">The destination palette structure.</param>
/// <param name="pairs">Array of 256 color pairs. (512 total)</param>
static void CreatePaletteTexturePair(LanternPalette& palette, ColorPair* pairs)
{
	using namespace d3d;
	if (device == nullptr)
	{
		throw std::exception("Device is null.");
	}

	CreateTexture(palette.diffuse);
	CreateTexture(palette.specular);

	auto diffuse_rect = GetTextureRect(palette.diffuse);
	auto diffuse_data = (NJS_COLOR*)diffuse_rect.pBits;

	auto specular_rect = GetTextureRect(palette.specular);
	auto specular_data = (NJS_COLOR*)specular_rect.pBits;

	for (size_t i = 0; i < 256; i++)
	{
		const auto& pair = pairs[i];
		auto& diffuse = diffuse_data[i];
		auto& specular = specular_data[i];

		diffuse = pair.diffuse;
		specular = pair.specular;
	}

	palette.diffuse->UnlockRect(0);
	palette.specular->UnlockRect(0);
}

/*
 * I'm explicitly using brackets in these switch statements
 * to make the code formatter happy. Otherwise it indents
 * comments and it looks really weird.
 */

static bool UseTimeOfDay(Uint32 level, Uint32 act)
{
	if (level < LevelIDs_StationSquare || level >= LevelIDs_Past && level <= LevelIDs_SandHill)
	{
		return false;
	}

	switch (level)
	{
		default:
		{
			return true;
		}

		// Time of day shouldn't be adjusted for
		// the entrance to Final Egg.
		case LevelIDs_MysticRuins:
		{
			return act != 3;
		}

		// Egg Carrier is not affected by time of day.
		case LevelIDs_EggCarrierInside:
		case LevelIDs_EggCarrierOutside:
		{
			return false;
		}

		// Egg Walker takes place in Station Square at night,
		// so it shouldn't be adjusted for time of day.
		case LevelIDs_EggWalker:
		{
			return false;
		}

		// These two Chao Gardens don't adjust for time of day
		// on the Dreamcast, and it doesn't work well with the
		// vanilla SADX landtables.
		case LevelIDs_SSGarden:
		case LevelIDs_ECGarden:
		{
			return false;
		}
	}
}

/// <summary>
/// Overwrites light directions for the current stage with the specified direction.
/// </summary>
/// <param name="dir">The direction with which to overwrite.</param>
void UpdateLightDirections(const NJS_VECTOR& dir)
{
	int level = CurrentLevel;
	int act = CurrentAct;
	GetTimeOfDayLevelAndAct(&level, &act);

	int n = 0;
	for (StageLightData* i = GetStageLight(level, act, n); i != nullptr; i = GetStageLight(level, act, ++n))
	{
		i->xyz = dir;
	}
}

bool LanternInstance::use_palette = false;
float LanternInstance::blend_factor = 0.0f;

bool LanternInstance::UsePalette()
{
	return use_palette;
}

/// <summary>
/// Returns a string in the format "_[0-9]", "1[A-Z]", "2[A-Z]", etc.
/// </summary>
/// <param name="level">Current level.</param>
/// <param name="act">Current act.</param>
/// <returns>A string containing the properly formatted PL level ID.</returns>
std::string LanternInstance::PaletteId(Sint32 level, Sint32 act)
{
	// TODO: Provide a method for other mods to handle this to allow for custom palettes.

	if (UseTimeOfDay(level, act))
	{
		// Station Square (unlike other sane adventure
		// fields) uses arbitrary numbers for time of day.
		if (level == LevelIDs_StationSquare)
		{
			switch (GetTimeOfDay())
			{
				default:
					act = 4;
					break;
				case 1:
					act = 1;
					break;
				case 2:
					act = 3;
					break;
			}
		}
		else
		{
			act = GetTimeOfDay();
		}
	}
	else
	{
		switch (level)
		{
			default:
			{
				break;
			}

			// Lost World act 3 uses act 1's palette.
			case LevelIDs_LostWorld:
			{
				if (act == 2)
				{
					act = 0;
				}

				break;
			}

			// Egg Walker takes place in Station Square at night.
			case LevelIDs_EggWalker:
			{
				level = LevelIDs_StationSquare;
				act = 3;
				break;
			}

			// Egg Carrier Exterior uses Emerald Coast's palette if it's on the ocean,
			// otherwise it just uses a single palette, unaffected by time of day.
			case LevelIDs_EggCarrierOutside:
			{
				int flag;

				switch (CurrentCharacter)
				{
					default:
						flag = EventFlags_Sonic_EggCarrierSunk;
						break;
					case Characters_Tails:
						flag = EventFlags_Tails_EggCarrierSunk;
						break;
					case Characters_Knuckles:
						flag = EventFlags_Knuckles_EggCarrierSunk;
						break;
					case Characters_Amy:
						flag = EventFlags_Amy_EggCarrierSunk;
						break;
					case Characters_Big:
						flag = EventFlags_Big_EggCarrierSunk;
						break;
					case Characters_Gamma:
						flag = EventFlags_Gamma_EggCarrierSunk;
						break;
				}

				if (EventFlagArray[flag] != 0)
				{
					level = LevelIDs_EmeraldCoast;
					act = 0;
				}
				else
				{
					act = 0;
				}
				break;
			}
		}
	}

	std::stringstream result;

	if (level < 10)
	{
		result << "_" << level << act;
		return result.str();
	}

	auto n = (level - 10) / 26;

	if (n > 9)
	{
		throw std::exception("Level ID out of range.");
	}

	!n ? result << "_" : result << n;

	auto i = (char)(level - (10 + 26 * n) + 'A');
	result << i << act;
	return result.str();
}

LanternInstance::LanternInstance(EffectParameter<IDirect3DTexture9*>* diffuse, EffectParameter<IDirect3DTexture9*>* specular)
	: diffuse_handle(diffuse), specular_handle(specular),
	  blend_type(-1), last_time(-1), last_act(-1), last_level(-1)
{
	for (auto& i : palette)
	{
		i = {};
	}
}

LanternInstance::LanternInstance(LanternInstance&& inst) noexcept
	: diffuse_handle(inst.diffuse_handle), specular_handle(inst.specular_handle),
	  blend_type(inst.blend_type), last_time(inst.last_time), last_act(inst.last_act), last_level(inst.last_level)
{
	for (int i = 0; i < 8; i++)
	{
		palette[i] = inst.palette[i];
		inst.palette[i] = {};
	}
}

LanternInstance& LanternInstance::operator=(LanternInstance&& inst) noexcept
{
	diffuse_handle  = inst.diffuse_handle;
	specular_handle = inst.specular_handle;
	last_time       = inst.last_time;
	last_act        = inst.last_act;
	last_level      = inst.last_level;
	blend_type      = inst.blend_type;

	inst.diffuse_handle = nullptr;
	inst.specular_handle = nullptr;

	for (int i = 0; i < 8; i++)
	{
		palette[i] = inst.palette[i];
		inst.palette[i] = {};
	}

	return *this;
}

LanternInstance::~LanternInstance()
{
	for (auto& i : palette)
	{
		if (i.diffuse)
		{
			i.diffuse->Release();
		}

		if (i.specular)
		{
			i.specular->Release();
		}
	}
}

void LanternInstance::SetLastLevel(Sint32 level, Sint32 act)
{
	last_level = level;
	last_act = act;
}

/// <summary>
/// Loads palette parameter data (light direction) from the specified path.
/// </summary>
/// <param name="path">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool ILantern::LoadSource(const std::string& path)
{
	bool result = true;
	auto file = std::ifstream(path, std::ios::binary);

	NJS_VECTOR dir;

	if (!file.is_open())
	{
		PrintDebug("[lantern] Lantern source not found: %s\n", path.c_str());
		dir = { 0.0f, -1.0f, 0.0f };
		result = false;
	}
	else
	{
		PrintDebug("[lantern] Loading lantern source: %s\n", path.c_str());

		Angle yaw, roll;

		file.seekg(0x5A0);
		file.read((char*)&yaw, sizeof(Angle));
		file.read((char*)&roll, sizeof(Angle));
		file.close();

		NJS_MATRIX m;
		auto _m = &m;

		njUnitMatrix(_m);
		njRotateY(_m, yaw);
		njRotateZ(_m, roll);

		// Default light direction is down, so we want to rotate relative to that.
		static const NJS_VECTOR vs = { 0.0f, -1.0f, 0.0f };
		njCalcVector(_m, (NJS_VECTOR*)&vs, &dir);
	}

	UpdateLightDirections(dir);
	return result;
}

/// <summary>
/// Loads palette parameter data (light direction) for the specified stage/act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::LoadSource(Sint32 level, Sint32 act) const
{
	std::stringstream name;
	name << globals::system << "SL" << PaletteId(level, act) << "B.BIN";
	return ILantern::LoadSource(name.str());
}

bool LanternInstance::LoadFiles()
{
	return LoadFiles(*this);
}

/// <summary>
/// Loads palette data from the specified path.
/// </summary>
/// <param name="level">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::LoadPalette(const std::string& path)
{
	auto file = std::ifstream(path, std::ios::binary);

	if (!file.is_open())
	{
		PrintDebug("[lantern] Lantern palette not found: %s\n", path.c_str());
		return false;
	}

	PrintDebug("[lantern] Loading lantern palette: %s\n", path.c_str());

	std::vector<ColorPair> colorData;

	do
	{
		ColorPair pair = {};
		file.read((char*)&pair.diffuse, sizeof(NJS_COLOR));
		file.read((char*)&pair.specular, sizeof(NJS_COLOR));
		colorData.push_back(pair);
	} while (!file.eof());

	file.close();

	for (size_t i = 0; i < 8; i++)
	{
		auto index = i * 256;
		if (index < colorData.size() && index + 256 < colorData.size())
		{
			CreatePaletteTexturePair(palette[i], &colorData[index]);
		}
		else
		{
			auto& pair = palette[i];

			if (pair.diffuse)
			{
				pair.diffuse->Release();
				pair.diffuse = nullptr;
			}

			if (pair.specular)
			{
				pair.specular->Release();
				pair.specular = nullptr;
			}
		}
	}

	return true;
}

/// <summary>
/// Loads palette data for the specified stage and act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::LoadPalette(Sint32 level, Sint32 act)
{
	std::stringstream name;
	name << globals::system << "PL" << PaletteId(level, act) << "B.BIN";
	return LoadPalette(name.str());
}

/// <summary>
/// Loads the lantern palette and source files for the current level if it (or the time of day) has changed.
/// </summary>
bool LanternInstance::LoadFiles(LanternInstance& instance)
{
	// TODO: remove this function
	// HACK: this is bad
	if (CurrentLevel == LevelIDs_SkyDeck)
	{
		return true;
	}

	auto time = GetTimeOfDay();

	for (int i = CurrentAct; i >= 0; i--)
	{
		int level = CurrentLevel;
		int act = i;

		GetTimeOfDayLevelAndAct(&level, &act);

		if (level == instance.last_level && act == instance.last_act && time == instance.last_time)
		{
			return false;
		}

		if (!instance.LoadPalette(CurrentLevel, i))
		{
			continue;
		}

		instance.LoadSource(CurrentLevel, i);

		instance.last_time  = time;
		instance.last_level = level;
		instance.last_act   = act;

		use_palette = false;
		d3d::do_effect = false;
		return true;
	}

	return false;
}

void LanternInstance::SetBlendFactor(float f)
{
	if (!d3d::effect)
	{
		return;
	}

	param::BlendFactor = f;
	blend_factor = f;
}

void LanternInstance::set_diffuse(Sint32 diffuse) const
{
	*diffuse_handle = palette[diffuse].diffuse;
}

void LanternInstance::set_specular(Sint32 specular) const
{
	*specular_handle = palette[specular].specular;
}

/// <summary>
/// Selects a diffuse and specular palette index based on the given SADX light type and material flags.
/// </summary>
/// <param name="type">SADX light type.</param>
/// <param name="flags">Material flags.</param>
void LanternInstance::SetPalettes(Sint32 type, Uint32 flags)
{
	auto pad = ControllerPointers[0];
	if (d3d::effect == nullptr || pad && pad->HeldButtons & Buttons_Z)
	{
		use_palette = false;
		d3d::do_effect = false;
		return;
	}

	// [0] 1-2 level geometry
	// [1] 3-4 objects (springs etc.)
	// [2] 5-6 player models & misc objects
	// [3] 7-8 Tails' shoes and jump ball (sometimes? probably level-specific)

	// type:
	// 0: stage
	// 2: character
	// 4: shiny thing (super sonic, gamma)
	// 6: enemy???

	Sint32 diffuse = -1;
	Sint32 specular = -1;

	globals::light_type = type;
	bool ignore_specular = !!(flags & NJD_FLAG_IGNORE_SPECULAR);

	switch (type)
	{
		case 0:
			diffuse = 0;
			specular = (ignore_specular && !(flags & NJD_FLAG_USE_ENV)) ? 0 : 1;
			globals::light_dir = *(NJS_VECTOR*)&Direct3D_CurrentLight.Direction;
			break;

		case 2:
		case 4:
			diffuse = 2;
			specular = ignore_specular ? 2 : 3;
			break;

		case 6:
			diffuse = 0;
			specular = 1;

		default:
			break;
	}

	// blend_type is used exclusively for local blending
	if (blend_type > -1)
	{
		// not calling SetBlendFactor because the value is saved
		// and restored when the light type matches the target type.
		if (type == blend_type)
		{
			param::BlendFactor = blend_factor;
		}
		else if (blend_factor != 0.0f)
		{
			param::BlendFactor = 0.0f;
		}
	}

	set_diffuse(diffuse);
	set_specular(specular);

	d3d::do_effect = use_palette = diffuse > -1 && specular > -1;
}

// hard coded crap
void LanternInstance::SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular)
{
	if (type < 0)
	{
		blend_type = -1;
		param::DiffusePaletteB = nullptr;
		param::SpecularPaletteB = nullptr;
		return;
	}

	blend_type = type;

	if (diffuse > -1)
	{
		param::DiffusePaletteB = palette[diffuse].diffuse;
	}

	if (specular > -1)
	{
		param::SpecularPaletteB = palette[specular].specular;
	}
}

// Collection

void LanternCollection::SetLastLevel(Sint32 level, Sint32 act)
{
	for (auto& i : instances)
	{
		i.SetLastLevel(level, act);
	}
}

bool LanternCollection::LoadPalette(Sint32 level, Sint32 act)
{
	size_t count = 0;

	for (auto& i : instances)
	{
		if (i.LoadPalette(level, act))
		{
			++count;
		}
	}

	return count == instances.size();
}

bool LanternCollection::LoadPalette(const std::string& path)
{
	size_t count = 0;

	for (auto& i : instances)
	{
		if (i.LoadPalette(path))
		{
			++count;
		}
	}

	return count == instances.size();
}

bool LanternCollection::LoadSource(Sint32 level, Sint32 act) const
{
	size_t count = 0;

	for (auto& i : instances)
	{
		if (i.LoadSource(level, act))
		{
			++count;
		}
	}

	return count == instances.size();
}

bool LanternCollection::LoadFiles()
{
	size_t count = 0;

	if (instances.empty())
	{
		instances.push_back(LanternInstance(&param::DiffusePalette, &param::SpecularPalette));
	}

	for (auto& i : instances)
	{
		if (i.LoadFiles())
		{
			++count;
		}
	}

	return count == instances.size();
}

void LanternCollection::SetPalettes(Sint32 type, Uint32 flags)
{
	for (auto& i : instances)
	{
		i.SetPalettes(type, flags);
	}
}

void LanternCollection::SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular)
{
	for (auto& i : instances)
	{
		i.SetSelfBlend(type, diffuse, specular);
	}
}

size_t LanternCollection::Add(LanternInstance& src)
{
	instances.push_back(std::move(src));
	return instances.size() - 1;
}

void LanternCollection::Remove(size_t index)
{
	instances.erase(instances.begin() + index);
}
