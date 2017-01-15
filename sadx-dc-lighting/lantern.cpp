#include "stdafx.h"

#include <d3d9.h>
#include <exception>
#include <string>
#include <sstream>
#include <vector>

#include "d3d.h"
#include <SADXModLoader.h>

#include "lantern.h"
#include "globals.h"

#ifdef _DEBUG
#include "datapointers.h"
#endif

#pragma pack(push, 1)
struct ColorPair
{
	NJS_COLOR diffuse, specular;
};
#pragma pack(pop)

static const ColorPair default_palette[256] =
{
	{ 0xFFFFFFFF, 0xFFC8C8C8 }, { 0xFFFFFFFF, 0xFFC0C0C0 },
	{ 0xFFFFFFFF, 0xFFB8B8B8 }, { 0xFFFFFFFF, 0xFFB0B0B0 },
	{ 0xFFFFFFFF, 0xFFA8A8A8 }, { 0xFFFFFFFF, 0xFFA0A0A0 },
	{ 0xFFFFFFFF, 0xFF989898 }, { 0xFFFFFFFF, 0xFF909090 },
	{ 0xFFFFFFFF, 0xFF888988 }, { 0xFFFFFFFF, 0xFF828382 },
	{ 0xFFFFFFFF, 0xFF7C7D7C }, { 0xFFFFFFFF, 0xFF767776 },
	{ 0xFFFFFFFF, 0xFF707170 }, { 0xFFFFFFFF, 0xFF6A6B6A },
	{ 0xFFFFFFFF, 0xFF646564 }, { 0xFFFFFFFF, 0xFF5E5F5E },
	{ 0xFFFFFFFF, 0xFF585A58 }, { 0xFFFEFEFE, 0xFF565856 },
	{ 0xFFFDFDFD, 0xFF545654 }, { 0xFFFCFCFC, 0xFF525452 },
	{ 0xFFFBFBFB, 0xFF515351 }, { 0xFFFAFAFA, 0xFF4F514F },
	{ 0xFFFAFAFA, 0xFF4D4F4D }, { 0xFFF9F9F9, 0xFF4C4D4C },
	{ 0xFFF8F8F8, 0xFF4A4C4A }, { 0xFFF7F7F7, 0xFF484A48 },
	{ 0xFFF6F6F6, 0xFF474847 }, { 0xFFF6F6F6, 0xFF454645 },
	{ 0xFFF5F5F5, 0xFF434543 }, { 0xFFF4F4F4, 0xFF424342 },
	{ 0xFFF3F3F3, 0xFF404140 }, { 0xFFF2F2F2, 0xFF3E3F3E },
	{ 0xFFF2F2F2, 0xFF3D3E3D }, { 0xFFF1F1F1, 0xFF3C3D3C },
	{ 0xFFF0F0F0, 0xFF3B3C3B }, { 0xFFEFEFEF, 0xFF3A3B3A },
	{ 0xFFEEEEEE, 0xFF393A39 }, { 0xFFEDEDED, 0xFF383938 },
	{ 0xFFEDEDED, 0xFF383838 }, { 0xFFECECEC, 0xFF373737 },
	{ 0xFFEBEBEB, 0xFF363736 }, { 0xFFEAEAEA, 0xFF353635 },
	{ 0xFFE9E9E9, 0xFF343534 }, { 0xFFE9E9E9, 0xFF333433 },
	{ 0xFFE8E8E8, 0xFF333333 }, { 0xFFE7E7E7, 0xFF323232 },
	{ 0xFFE6E6E6, 0xFF313131 }, { 0xFFE5E5E5, 0xFF303030 },
	{ 0xFFE5E5E5, 0xFF2F302F }, { 0xFFE3E3E3, 0xFF2E2F2E },
	{ 0xFFE2E2E2, 0xFF2E2E2E }, { 0xFFE0E0E0, 0xFF2D2D2D },
	{ 0xFFDFDFDF, 0xFF2C2C2C }, { 0xFFDEDEDE, 0xFF2B2B2B },
	{ 0xFFDCDCDC, 0xFF2A2A2A }, { 0xFFDBDBDB, 0xFF292929 },
	{ 0xFFD9D9D9, 0xFF292929 }, { 0xFFD8D8D8, 0xFF292929 },
	{ 0xFFD7D7D7, 0xFF282828 }, { 0xFFD5D5D5, 0xFF282828 },
	{ 0xFFD4D4D4, 0xFF282828 }, { 0xFFD2D2D2, 0xFF272727 },
	{ 0xFFD1D1D1, 0xFF272727 }, { 0xFFD0D0D0, 0xFF272727 },
	{ 0xFFCECECE, 0xFF262626 }, { 0xFFCDCDCD, 0xFF262626 },
	{ 0xFFCBCBCB, 0xFF262626 }, { 0xFFCACACA, 0xFF262626 },
	{ 0xFFC9C9C9, 0xFF252525 }, { 0xFFC7C7C7, 0xFF252525 },
	{ 0xFFC6C6C6, 0xFF252525 }, { 0xFFC4C4C4, 0xFF242424 },
	{ 0xFFC3C3C3, 0xFF242424 }, { 0xFFC2C2C2, 0xFF242424 },
	{ 0xFFC0C0C0, 0xFF242424 }, { 0xFFBFBFBF, 0xFF232323 },
	{ 0xFFBEBEBE, 0xFF232323 }, { 0xFFBCBCBC, 0xFF232323 },
	{ 0xFFBBBBBB, 0xFF222222 }, { 0xFFB9B9B9, 0xFF222222 },
	{ 0xFFB8B8B8, 0xFF222222 }, { 0xFFB7B7B7, 0xFF212121 },
	{ 0xFFB5B5B5, 0xFF212121 }, { 0xFFB4B4B4, 0xFF212121 },
	{ 0xFFB2B2B2, 0xFF212121 }, { 0xFFB1B1B1, 0xFF202020 },
	{ 0xFFB0B0B0, 0xFF202020 }, { 0xFFAEAEAE, 0xFF202020 },
	{ 0xFFADADAD, 0xFF1F1F1F }, { 0xFFABABAB, 0xFF1F1F1F },
	{ 0xFFAAAAAA, 0xFF1F1F1F }, { 0xFFA9A9A9, 0xFF1F1F1F },
	{ 0xFFA7A7A7, 0xFF1E1E1E }, { 0xFFA6A6A6, 0xFF1E1E1E },
	{ 0xFFA4A4A4, 0xFF1E1E1E }, { 0xFFA3A3A3, 0xFF1D1D1D },
	{ 0xFFA2A2A2, 0xFF1D1D1D }, { 0xFFA0A0A0, 0xFF1C1C1C },
	{ 0xFF9F9F9F, 0xFF1C1C1C }, { 0xFF9D9D9D, 0xFF1C1C1C },
	{ 0xFF9C9C9C, 0xFF1C1C1C }, { 0xFF9B9B9B, 0xFF1C1C1C },
	{ 0xFF999999, 0xFF1C1C1C }, { 0xFF989898, 0xFF1C1C1C },
	{ 0xFF979797, 0xFF1C1C1C }, { 0xFF969696, 0xFF1C1C1C },
	{ 0xFF959595, 0xFF1C1C1C }, { 0xFF949494, 0xFF1C1C1C },
	{ 0xFF939393, 0xFF1C1C1C }, { 0xFF929292, 0xFF1C1C1C },
	{ 0xFF919191, 0xFF1C1C1C }, { 0xFF909090, 0xFF1C1C1C },
	{ 0xFF8F8F8F, 0xFF1C1C1C }, { 0xFF8E8E8E, 0xFF1C1C1C },
	{ 0xFF8D8D8D, 0xFF1C1C1C }, { 0xFF8C8C8C, 0xFF1C1C1C },
	{ 0xFF8B8B8B, 0xFF1C1C1C }, { 0xFF8A8A8A, 0xFF1C1C1C },
	{ 0xFF898989, 0xFF1C1C1C }, { 0xFF888888, 0xFF1C1C1C },
	{ 0xFF878787, 0xFF1C1C1C }, { 0xFF868686, 0xFF1C1C1C },
	{ 0xFF858585, 0xFF1C1C1C }, { 0xFF848484, 0xFF1C1C1C },
	{ 0xFF838383, 0xFF1C1C1C }, { 0xFF828282, 0xFF1C1C1C },
	{ 0xFF818181, 0xFF1C1C1C }, { 0xFF808080, 0xFF1C1C1C },
	{ 0xFF7F7F7F, 0xFF1C1C1C }, { 0xFF7E7E7E, 0xFF1C1C1C },
	{ 0xFF7D7D7D, 0xFF1C1C1C }, { 0xFF7C7C7C, 0xFF1C1C1C },
	{ 0xFF7B7B7B, 0xFF1C1C1C }, { 0xFF7A7A7A, 0xFF1C1C1C },
	{ 0xFF797979, 0xFF1C1C1C }, { 0xFF787878, 0xFF1C1C1C },
	{ 0xFF777777, 0xFF1C1C1C }, { 0xFF767676, 0xFF1C1C1C },
	{ 0xFF757575, 0xFF1C1C1C }, { 0xFF747474, 0xFF1C1C1C },
	{ 0xFF737373, 0xFF1C1C1C }, { 0xFF727272, 0xFF1C1C1C },
	{ 0xFF717171, 0xFF1C1C1C }, { 0xFF707070, 0xFF1C1C1C },
	{ 0xFF6F6F6F, 0xFF1C1C1C }, { 0xFF6E6E6E, 0xFF1C1C1C },
	{ 0xFF6D6D6D, 0xFF1C1C1C }, { 0xFF6C6C6C, 0xFF1C1C1C },
	{ 0xFF6B6B6B, 0xFF1C1C1C }, { 0xFF6A6A6A, 0xFF1C1C1C },
	{ 0xFF696969, 0xFF1C1C1C }, { 0xFF686868, 0xFF1C1C1C },
	{ 0xFF676767, 0xFF1C1C1C }, { 0xFF666666, 0xFF1C1C1C },
	{ 0xFF656565, 0xFF1C1C1C }, { 0xFF646464, 0xFF1C1C1C },
	{ 0xFF636363, 0xFF1C1C1C }, { 0xFF626262, 0xFF1C1C1C },
	{ 0xFF616161, 0xFF1C1C1C }, { 0xFF606060, 0xFF1C1C1C },
	{ 0xFF5F5F5F, 0xFF1C1C1C }, { 0xFF5E5E5E, 0xFF1C1C1C },
	{ 0xFF5D5D5D, 0xFF1C1C1C }, { 0xFF5C5C5C, 0xFF1C1C1C },
	{ 0xFF5B5B5B, 0xFF1C1C1C }, { 0xFF5A5A5A, 0xFF1C1C1C },
	{ 0xFF595959, 0xFF1C1C1C }, { 0xFF585858, 0xFF1C1C1C },
	{ 0xFF575757, 0xFF1C1C1C }, { 0xFF565656, 0xFF1C1C1C },
	{ 0xFF555555, 0xFF1C1C1C }, { 0xFF545454, 0xFF1C1C1C },
	{ 0xFF535353, 0xFF1C1C1C }, { 0xFF525252, 0xFF1C1C1C },
	{ 0xFF515151, 0xFF1C1C1C }, { 0xFF505050, 0xFF1C1C1C },
	{ 0xFF4F4F4F, 0xFF1C1C1C }, { 0xFF4E4E4E, 0xFF1C1C1C },
	{ 0xFF4D4D4D, 0xFF1C1C1C }, { 0xFF4C4C4C, 0xFF1C1C1C },
	{ 0xFF4B4B4B, 0xFF1C1C1C }, { 0xFF4A4A4A, 0xFF1C1C1C },
	{ 0xFF494949, 0xFF1C1C1C }, { 0xFF484848, 0xFF1C1C1C },
	{ 0xFF474747, 0xFF1C1C1C }, { 0xFF464646, 0xFF1C1C1C },
	{ 0xFF464646, 0xFF1C1C1C }, { 0xFF454545, 0xFF1C1C1C },
	{ 0xFF454545, 0xFF1C1C1C }, { 0xFF444444, 0xFF1C1C1C },
	{ 0xFF444444, 0xFF1C1C1C }, { 0xFF444444, 0xFF1C1C1C },
	{ 0xFF434343, 0xFF1C1C1C }, { 0xFF434343, 0xFF1C1C1C },
	{ 0xFF424242, 0xFF1C1C1C }, { 0xFF424242, 0xFF1C1C1C },
	{ 0xFF424242, 0xFF1C1C1C }, { 0xFF414141, 0xFF1C1C1C },
	{ 0xFF414141, 0xFF1C1C1C }, { 0xFF404040, 0xFF1C1C1C },
	{ 0xFF404040, 0xFF1C1C1C }, { 0xFF3F3F3F, 0xFF1C1C1C },
	{ 0xFF3F3F3F, 0xFF1C1C1C }, { 0xFF3F3F3F, 0xFF1C1C1C },
	{ 0xFF3E3E3E, 0xFF1C1C1C }, { 0xFF3E3E3E, 0xFF1C1C1C },
	{ 0xFF3D3D3D, 0xFF1C1C1C }, { 0xFF3D3D3D, 0xFF1C1C1C },
	{ 0xFF3D3D3D, 0xFF1C1C1C }, { 0xFF3C3C3C, 0xFF1D1D1D },
	{ 0xFF3C3C3C, 0xFF1D1D1D }, { 0xFF3B3B3B, 0xFF1D1D1D },
	{ 0xFF3B3B3B, 0xFF1E1E1E }, { 0xFF3A3A3A, 0xFF1E1E1E },
	{ 0xFF3A3A3A, 0xFF1E1E1E }, { 0xFF3A3A3A, 0xFF1F1F1F },
	{ 0xFF393939, 0xFF1F1F1F }, { 0xFF393939, 0xFF1F1F1F },
	{ 0xFF393939, 0xFF202020 }, { 0xFF393939, 0xFF202020 },
	{ 0xFF393939, 0xFF202020 }, { 0xFF3A3A3A, 0xFF212121 },
	{ 0xFF3A3A3A, 0xFF212121 }, { 0xFF3A3A3A, 0xFF212121 },
	{ 0xFF3A3A3A, 0xFF222222 }, { 0xFF3A3A3A, 0xFF222222 },
	{ 0xFF3B3B3B, 0xFF232323 }, { 0xFF3B3B3B, 0xFF242424 },
	{ 0xFF3B3B3B, 0xFF242424 }, { 0xFF3B3B3B, 0xFF252525 },
	{ 0xFF3B3B3B, 0xFF262626 }, { 0xFF3C3C3C, 0xFF262626 },
	{ 0xFF3C3C3C, 0xFF272727 }, { 0xFF3D3D3D, 0xFF282828 },
	{ 0xFF3F3F3F, 0xFF282828 }, { 0xFF404040, 0xFF292929 },
	{ 0xFF424242, 0xFF2A2A2A }, { 0xFF434343, 0xFF2A2A2A },
	{ 0xFF454545, 0xFF2B2B2B }, { 0xFF464646, 0xFF2C2C2C },
	{ 0xFF484848, 0xFF2D2D2D }, { 0xFF494949, 0xFF2F2F2F },
	{ 0xFF4B4B4B, 0xFF313131 }, { 0xFF4C4C4C, 0xFF333333 },
	{ 0xFF4E4E4E, 0xFF353535 }, { 0xFF4F4F4F, 0xFF373737 },
	{ 0xFF515151, 0xFF393939 }, { 0xFF525252, 0xFF3B3B3B },
	{ 0xFF545454, 0xFF3E3E3E }, { 0xFF575757, 0xFF414141 },
	{ 0xFF595959, 0xFF454545 }, { 0xFF5C5C5C, 0xFF494949 },
	{ 0xFF5F5F5F, 0xFF4C4C4C }, { 0xFF626262, 0xFF505050 },
	{ 0xFF656565, 0xFF545454 }, { 0xFF686868, 0xFF585858 },
};

static LanternPalette* defaultPalette = nullptr;

bool SourceLight_t::operator==(const SourceLight_t& rhs) const
{
	return !memcmp(this, &rhs, sizeof(SourceLight_t)); 
}

bool SourceLight_t::operator!=(const SourceLight_t& rhs) const
{
	return !operator==(rhs);
}

template<>
void EffectParameter<SourceLight_t>::Commit()
{
	if (Modified())
	{
		(*effect)->SetValue(handle, &current, sizeof(SourceLight_t));
		Clear();
	}
}

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
static void CreatePaletteTexturePair(LanternPalette& palette, const ColorPair* pairs)
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
	: diffuse_param(diffuse), specular_param(specular),
	  blend_type(-1), last_time(-1), last_act(-1), last_level(-1)
{
	for (auto& i : palette)
	{
		i = {};
	}
}

LanternInstance::LanternInstance(LanternInstance&& inst) noexcept
	: diffuse_param(inst.diffuse_param), specular_param(inst.specular_param),
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
	diffuse_param  = inst.diffuse_param;
	specular_param = inst.specular_param;
	last_time      = inst.last_time;
	last_act       = inst.last_act;
	last_level     = inst.last_level;
	blend_type     = inst.blend_type;

	inst.diffuse_param = nullptr;
	inst.specular_param = nullptr;

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
			i.diffuse = nullptr;
		}

		if (i.specular)
		{
			i.specular->Release();
			i.specular = nullptr;
		}
	}
}

void LanternInstance::SetLastLevel(Sint32 level, Sint32 act)
{
	last_level = level;
	last_act = act;
}

static SourceLight SourceLights[16] = {};

/// <summary>
/// Loads palette parameter data (light direction) from the specified path.
/// </summary>
/// <param name="path">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::LoadSource(const std::string& path)
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

		for (int i = 0; i < 16; i++)
		{
			file.read((char*)&SourceLights[i], sizeof(SourceLight));
		}

		file.close();

		param::SourceLight = SourceLights[15].stage;

		NJS_MATRIX m;

		njUnitMatrix(&m);
		njRotateY(&m, SourceLights[15].stage.y);
		njRotateZ(&m, SourceLights[15].stage.z);

		// Default light direction is down, so we want to rotate relative to that.
		static const NJS_VECTOR vs = { 0.0f, -1.0f, 0.0f };
		njCalcVector(&m, (NJS_VECTOR*)&vs, &dir);
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
bool LanternInstance::LoadSource(Sint32 level, Sint32 act)
{
	std::stringstream name;
	name << globals::system << "SL" << PaletteId(level, act) << "B.BIN";
	return LoadSource(name.str());
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
	// Sky Deck needs to manage its own palette.
	// TODO: something better than this
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
	if (diffuse >= 0)
	{
		*diffuse_param = palette[diffuse].diffuse;
	}
}

void LanternInstance::set_specular(Sint32 specular) const
{
	if (specular >= 0)
	{
		*specular_param = palette[specular].specular;
	}
}

/// <summary>
/// Selects a diffuse and specular palette index based on the given SADX light type and material flags.
/// </summary>
/// <param name="type">SADX light type.</param>
/// <param name="flags">Material flags.</param>
void LanternInstance::SetPalettes(Sint32 type, Uint32 flags)
{
#ifdef _DEBUG
	auto pad = ControllerPointers[0];
#endif

	if (d3d::effect == nullptr
#ifdef _DEBUG
		|| pad && pad->HeldButtons & Buttons_Z
#endif
	)
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
#ifdef _DEBUG
			globals::light_dir = *(NJS_VECTOR*)&Direct3D_CurrentLight.Direction;
#endif
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

	//set_diffuse(diffuse);
	//set_specular(specular);

	if (!defaultPalette)
	{
		defaultPalette = new LanternPalette();
		CreatePaletteTexturePair(*defaultPalette, default_palette);
	}

	param::UseSourceLight = true;
	param::SourceLight = SourceLights[15].stage;
	*diffuse_param = defaultPalette->diffuse;
	*specular_param = defaultPalette->specular;

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

bool LanternCollection::LoadSource(Sint32 level, Sint32 act)
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

bool LanternCollection::LoadSource(const std::string& path)
{
	size_t count = 0;

	for (auto& i : instances)
	{
		if (i.LoadSource(path))
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
