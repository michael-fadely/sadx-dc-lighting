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

static bool use_palette = false;
static LanternPalette palettes[8] = {};

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
inline void CreateTexture(IDirect3DTexture9** texture)
{
	if (FAILED(d3d::device->CreateTexture(1, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, nullptr)))
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
		throw std::exception("Device is null.");

	if (palette.diffuse)
	{
		palette.diffuse->Release();
		palette.diffuse = nullptr;
	}

	if (palette.specular)
	{
		palette.specular->Release();
		palette.specular = nullptr;
	}

	if (!palette.diffuse)
		CreateTexture(&palette.diffuse);
	if (!palette.specular)
		CreateTexture(&palette.specular);

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

bool UsePalette()
{
	return use_palette;
}

/// <summary>
/// Returns a string in the format "_[0-9]", "1[A-Z]", "2[A-Z]", etc.
/// </summary>
/// <param name="level">Current level.</param>
/// <param name="act">Current act.</param>
/// <returns>A string containing the properly formatted PL level ID.</returns>
std::string LanternPaletteId(Uint32 level, Uint32 act)
{
	// Special cases because sonic adventure:
	// Egg Walker specifically loads the night time Station Square palette.
	if (level == LevelIDs_EggWalker)
	{
		level = LevelIDs_StationSquare;
		act = 3;
	}
	else if (level >= LevelIDs_StationSquare && (level < LevelIDs_Past || level > LevelIDs_SandHill))
	{
		switch (level)
		{
			// Any other adventure field presumably uses time of day
			// in place of act for filenames (e.g Mystic Ruins).
			default:
				act = GetTimeOfDay();
				break;

				// Egg Carrier interior is not affected by time of day.
			case LevelIDs_EggCarrierInside:
				break;

				// Station Square, unlike other sane adventure fields,
				// uses arbitrary numbers for time of day.
			case LevelIDs_StationSquare:
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

				break;

				// Station Square Chao Garden only supports day and evening.
			case LevelIDs_SSGarden:
				act = min(1, GetTimeOfDay());
				break;

				// Egg Carrier Exterior uses Emerald Coast's palette if it's on the ocean,
				// otherwise it just uses a single palette, unaffected by time of day.
			case LevelIDs_EggCarrierOutside:
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

	if (!n)
	{
		result << "_";
	}
	else
	{
		result << n;
	}


	auto i = (char)(level - (10 + 26 * n) + 'A');
	result << i << act;
	return result.str();
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

/// <summary>
/// Loads palette parameter data (light direction) for the specified stage/act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LoadLanternSource(Uint32 level, Uint32 act)
{
	bool result = true;

	std::stringstream name;
	name << globals::system << "SL" << LanternPaletteId(level, act) << "B.BIN";
	auto file = std::ifstream(name.str(), std::ios::binary);

	NJS_VECTOR dir;

	if (!file.is_open())
	{
		dir = { 0.0f, -1.0f, 0.0f };
		result = false;
	}
	else
	{
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
/// Loads palette data for the specified stage and act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LoadLanternPalette(Uint32 level, Uint32 act)
{
	std::stringstream name;
	name << globals::system << "PL" << LanternPaletteId(level, act) << "B.BIN";
	auto file = std::ifstream(name.str(), std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

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
			CreatePaletteTexturePair(palettes[i], &colorData[index]);
		}
		else
		{
			auto& palette = palettes[i];

			if (palette.diffuse)
			{
				palette.diffuse->Release();
				palette.diffuse = nullptr;
			}

			if (palette.specular)
			{
				palette.specular->Release();
				palette.specular = nullptr;
			}
		}
	}

	return true;
}

/// <summary>
/// Loads the lantern palette and source files for the current level if it (or the time of day) has changed.
/// </summary>
void LoadLanternFiles()
{
	auto time = GetTimeOfDay();

	for (int i = CurrentAct; i >= 0; i--)
	{
		int level = CurrentLevel;
		int act = i;

		GetTimeOfDayLevelAndAct(&level, &act);

		if (level == globals::last_level && act == globals::last_act && time == globals::last_time)
			break;

		if (!LoadLanternPalette(CurrentLevel, i))
			continue;

		LoadLanternSource(CurrentLevel, i);

		globals::last_time = time;
		globals::last_level = level;
		globals::last_act = act;

		use_palette = false;
		d3d::do_effect = false;
		break;
	}
}

void SetPaletteLights(int type, bool common_object)
{
	globals::last_type = type;
	auto pad = ControllerPointers[0];
	if (d3d::effect == nullptr || pad && pad->HeldButtons & Buttons_Z)
	{
		use_palette = false;
		d3d::do_effect = false;
		return;
	}

	globals::light = true;

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

	switch (type)
	{
		case 0:
			diffuse = 0;
			specular = common_object ? 1 : 0;
			globals::light_dir = *(NJS_VECTOR*)&Direct3D_CurrentLight.Direction;
			break;

		case 2:
			diffuse = 2;
			specular = 2;
			break;

		case 4:
			diffuse = 2;
			specular = 3;
			break;

		case 6:
			diffuse = 0;
			specular = 1;

		default:
			break;
	}

	if (diffuse < 0 || specular < 0)
	{
		use_palette = false;
		d3d::do_effect = false;
		return;
	}

	d3d::effect->SetTexture("DiffusePalette", palettes[diffuse].diffuse);
	d3d::effect->SetTexture("SpecularPalette", palettes[specular].specular);
	use_palette = true;
}
