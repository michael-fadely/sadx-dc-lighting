#include "stdafx.h"

#include <atlbase.h>
#include <d3d9.h>

#include <algorithm>
#include <exception>
#include <string>
#include <sstream>
#include <vector>

#include "d3d.h"
#include <SADXModLoader.h>

#include "globals.h"

#ifdef _DEBUG
#include "datapointers.h"
#endif

#include "lantern.h"

#pragma pack(push, 1)
struct ColorPair
{
	NJS_COLOR diffuse, specular;
};
#pragma pack(pop)

bool SourceLight_t::operator==(const SourceLight_t& rhs) const
{
	return !memcmp(this, &rhs, sizeof(SourceLight_t)); 
}

bool SourceLight_t::operator!=(const SourceLight_t& rhs) const
{
	return !operator==(rhs);
}

template<>
bool EffectParameter<SourceLight_t>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetValue(handle, &current, sizeof(SourceLight_t));
		Clear();
		return true;
	}

	return false;
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

bool LanternInstance::diffuse_override       = false;
bool LanternInstance::diffuse_override_temp  = false;
bool LanternInstance::specular_override      = false;
bool LanternInstance::specular_override_temp = false;
bool LanternInstance::use_palette            = false;
float LanternInstance::blend_factor          = 0.0f;

bool LanternInstance::UsePalette()
{
	return use_palette;
}

float LanternInstance::BlendFactor()
{
	return blend_factor;
}

inline bool GameModeIngame()
{
	switch ((GameModes)GameMode)
	{
		case GameModes_Restart:
		case GameModes_Adventure_ActionStg:
		case GameModes_Adventure_Field:
		case GameModes_Trial:
		case GameModes_Mission:
		case GameModes_Restart2:
		case GameModes_StartAdventure:
		case GameModes_Adventure_Story:
			return true;

		default:
			return false;
	}
}

/// <summary>
/// Returns a string in the format "_[0-9]", "1[A-Z]", "2[A-Z]", etc.
/// </summary>
/// <param name="level">Current level.</param>
/// <param name="act">Current act.</param>
/// <returns>A string containing the properly formatted PL level ID.</returns>
std::string LanternInstance::PaletteId(Sint32 level, Sint32 act)
{
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

			case LevelIDs_HedgehogHammer:
			{
				if (GameModeIngame())
				{
					level = LevelIDs_EggCarrierInside;
					act = 2;
				}
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
				// Act 4 is the "Private Room" area. Act 3 and 5 (Captain's Room, Pool) both
				// use their act numbers for palette loading. In every other case, use act 0.
				if (act == 4)
				{
					act = 2;
				}
				else if (act < 3 || act > 5)
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

void LanternInstance::copy(LanternInstance& inst)
{
	atlas            = inst.atlas;
	diffuse_param    = inst.diffuse_param;
	specular_param   = inst.specular_param;
	blend_type       = inst.blend_type;
	last_time        = inst.last_time;
	last_act         = inst.last_act;
	last_level       = inst.last_level;
	diffuse_index    = inst.diffuse_index;
	specular_index   = inst.specular_index;
	diffuse_index_b  = inst.diffuse_index_b;
	specular_index_b = inst.specular_index_b;
}

LanternInstance::LanternInstance(EffectParameter<Texture>* atlas, EffectParameter<float>* diffuse_param, EffectParameter<float>* specular_param)
	: atlas(atlas), diffuse_param(diffuse_param), specular_param(specular_param)
{
}

LanternInstance::LanternInstance(LanternInstance&& inst) noexcept
{
	copy(inst);
}

LanternInstance& LanternInstance::operator=(LanternInstance&& inst) noexcept
{
	copy(inst);
	inst.atlas = nullptr;
	return *this;
}

LanternInstance::~LanternInstance()
{
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

#ifdef USE_SL
		param::SourceLight = SourceLights[15].stage;
#endif

		NJS_MATRIX m;

		njUnitMatrix(m);
		njRotateY(m, SourceLights[15].stage.y);
		njRotateZ(m, SourceLights[15].stage.z);

		// Default light direction is down, so we want to rotate relative to that.
		static const NJS_VECTOR vs = { 0.0f, -1.0f, 0.0f };
		njCalcVector(m, (NJS_VECTOR*)&vs, &dir);
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

/// <summary>
/// Loads palette data from the specified path.
/// </summary>
/// <param name="level">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::LoadPalette(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);

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

	Texture texture = nullptr;

	if (FAILED(d3d::device->CreateTexture(256, 16, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
	{
		throw std::exception("Failed to create palette texture!");
	}

	*atlas = texture;

	D3DLOCKED_RECT rect;
	if (FAILED(texture->LockRect(0, &rect, nullptr, 0)))
	{
		throw std::exception("Failed to lock texture rect!");
	}

	auto pixels = (NJS_COLOR*)rect.pBits;

	for (size_t i = 0; i < 8; i++)
	{
		auto index = i * 256;
		if (index >= colorData.size() || index + 256 >= colorData.size())
		{
			break;
		}

		auto y = 512 * i;
		for (size_t x = 0; x < 256; x++)
		{
			auto& diffuse = pixels[y + x];
			auto& specular = pixels[256 + y + x];
			const auto& color = colorData[index + x];
			diffuse = color.diffuse;
			specular = color.specular;
		}
	}

	texture->UnlockRect(0);

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

void LanternInstance::SetBlendFactor(float f)
{
	if (!d3d::effect)
	{
		return;
	}

	param::BlendFactor = f;
	blend_factor = f;
}

inline float _index_float(Sint32 i, Sint32 offset)
{
	return ((float)(2 * i + offset) + 0.5f) / 16.0f;
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
			specular = ignore_specular && (globals::landtable_specular || !(flags & NJD_FLAG_USE_ENV)) ? 0 : 1;

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

	if (!diffuse_override)
	{
		SetDiffuse(diffuse);
	}

	if (!specular_override)
	{
		SetSpecular(specular);
	}

	d3d::do_effect = use_palette = diffuse > -1 && specular > -1;
}

void LanternInstance::SetDiffuse(Sint32 n)
{
	if (n >= 0)
	{
		*diffuse_param = _index_float(n, 0);
	}

	diffuse_index = n;
}

void LanternInstance::SetSpecular(Sint32 n)
{
	if (n >= 0)
	{
		*specular_param = _index_float(n, 1);
	}

	specular_index = n;
}

Sint32 LanternInstance::GetDiffuse()
{
	return diffuse_index;
}

Sint32 LanternInstance::GetSpecular()
{
	return specular_index;
}

void LanternInstance::SetDiffuseB(Sint32 n)
{
	if (n > -1)
	{
		param::DiffuseIndexB = _index_float(n, 0);
	}

	diffuse_index_b = n;
}

void LanternInstance::SetSpecularB(Sint32 n)
{
	if (n > -1)
	{
		param::SpecularIndexB = _index_float(n, 1);
	}

	specular_index_b = n;
}

Sint32 LanternInstance::GetDiffuseB() const
{
	return diffuse_index_b;
}

Sint32 LanternInstance::GetSpecularB() const
{
	return specular_index_b;
}

// hard coded crap
void LanternInstance::SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular)
{
	if (type < 0)
	{
		blend_type = -1;
		param::PaletteB = nullptr;
		return;
	}

	blend_type = type;
	param::PaletteB = param::PaletteA;

	SetDiffuseB(diffuse);
	SetSpecularB(specular);
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

// TODO: solve duplicate code by having PL and SL classes which inherit a common interface
bool LanternCollection::RunPlCallbacks(Sint32 level, Sint32 act, Sint8 time)
{
	bool result = false;

	for (auto& cb : pl_callbacks)
	{
		auto path_ptr = cb(level, act);

		if (path_ptr == nullptr)
		{
			continue;
		}

		std::string path_str = path_ptr;

		for (auto& instance : instances)
		{
			if (level == instance.last_level && act == instance.last_act && time == instance.last_time)
			{
				result = true;
				break;
			}

			if (!instance.LoadPalette(path_str))
			{
				return false;
			}

			instance.last_time  = time;
			instance.last_level = level;
			instance.last_act   = act;
			result = true;
		}

		if (result)
		{
			break;
		}
	}

	return result;
}

bool LanternCollection::RunSlCallbacks(Sint32 level, Sint32 act, Sint8 time)
{
	bool result = false;

	for (auto& cb : sl_callbacks)
	{
		auto path_ptr = cb(level, act);

		if (path_ptr == nullptr)
		{
			continue;
		}

		std::string path_str = path_ptr;

		for (auto& instance : instances)
		{
			if (level == instance.last_level && act == instance.last_act && time == instance.last_time)
			{
				result = true;
				break;
			}

			if (!instance.LoadSource(path_str))
			{
				return false;
			}

			instance.last_time  = time;
			instance.last_level = level;
			instance.last_act   = act;
			result = true;
		}

		if (result)
		{
			break;
		}
	}

	return result;
}

bool LanternCollection::LoadFiles()
{
	auto time = GetTimeOfDay();
	size_t count = 0;

	if (instances.empty())
	{
		instances.emplace_back(&param::PaletteA, &param::DiffuseIndexA, &param::SpecularIndexA);
	}

	bool pl_handled = RunPlCallbacks(CurrentLevel, CurrentAct, time);
	bool sl_handled = RunSlCallbacks(CurrentLevel, CurrentAct, time);

	// No need to do automatic detection if a callback has
	// already provided valid paths to both PL and SL files.
	if (pl_handled && sl_handled)
	{
		return true;
	}

	// Sky Deck needs to manage its own palette.
	// TODO: something better than this
	if (CurrentLevel == LevelIDs_SkyDeck)
	{
		return true;
	}

	for (auto& instance : instances)
	{
		// This is a fallback for cases where stage acts
		// palettes from the previous acts.
		for (int i = CurrentAct; i >= 0; i--)
		{
			int level = CurrentLevel;
			int act = i;

			if (UseTimeOfDay(level, act))
			{
				GetTimeOfDayLevelAndAct(&level, &act);
			}

			if (!pl_handled && !sl_handled
				&& level == instance.last_level && act == instance.last_act && time == instance.last_time)
			{
				break;
			}

			if (!pl_handled && !instance.LoadPalette(CurrentLevel, i))
			{
				// Palette loading is critical for lighting, so
				// continue immediately on failure.
				continue;
			}

			if (!sl_handled)
			{
				// Source light loading on the other hand is not a
				// requirement, so failure is fine.
				instance.LoadSource(CurrentLevel, i);
			}

			instance.last_time  = time;
			instance.last_level = level;
			instance.last_act   = act;

			LanternInstance::use_palette = false;
			d3d::do_effect = false;
			++count;
			break;
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

void LanternCollection::SetDiffuse(Sint32 n)
{
	for (auto& i : instances)
	{
		i.SetDiffuse(n);
	}
}

void LanternCollection::SetSpecular(Sint32 n)
{
	for (auto& i : instances)
	{
		i.SetSpecular(n);
	}
}

void LanternCollection::SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular)
{
	for (auto& i : instances)
	{
		i.SetSelfBlend(type, diffuse, specular);
	}
}

void LanternCollection::callback_add(std::deque<lantern_load_cb>& c, lantern_load_cb callback)
{
	if (callback == nullptr)
	{
		return;
	}

	callback_del(c, callback);
	c.push_back(callback);
}

void LanternCollection::callback_del(std::deque<lantern_load_cb>& c, lantern_load_cb callback)
{
	remove(c.begin(), c.end(), callback);
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

void LanternCollection::AddPlCallback(lantern_load_cb callback)
{
	callback_add(pl_callbacks, callback);
}

void LanternCollection::RemovePlCallback(lantern_load_cb callback)
{
	callback_del(pl_callbacks, callback);
}

void LanternCollection::AddSlCallback(lantern_load_cb callback)
{
	callback_add(sl_callbacks, callback);
}

void LanternCollection::RemoveSlCallback(lantern_load_cb callback)
{
	callback_del(sl_callbacks, callback);
}
