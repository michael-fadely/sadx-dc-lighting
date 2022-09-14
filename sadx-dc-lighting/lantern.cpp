#include "stdafx.h"

#include <atlbase.h>
#include <d3d9.h>

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

#include "d3d.h"
#include <SADXModLoader.h>

#include "globals.h"
#include "datapointers.h"
#include "lantern.h"

// The game's palette data array which contains 8+2 palette pairs.
// The extra pairs were used on DC for backups via lig_cpyPalette.
// The PC version only uses one extra pair to store stage palettes.
// It is used in Gamma's briefing cutscene when the lights turn off.
// The extra pairs are never cleared on DC and in this mod.
DataArray(ColorPair[256], LSPAL, 0x3B12210, 10);

bool SourceLight_t::operator==(const SourceLight_t& rhs) const
{
	return !memcmp(this, &rhs, sizeof(SourceLight_t));
}

bool SourceLight_t::operator!=(const SourceLight_t& rhs) const
{
	return !operator==(rhs);
}

bool StageLight::operator==(const StageLight& rhs) const
{
	return specular == rhs.specular &&
	       multiplier == rhs.multiplier &&
	       !memcmp(&direction, &rhs.direction, sizeof(NJS_VECTOR)) &&
	       !memcmp(&diffuse, &rhs.diffuse, sizeof(NJS_VECTOR)) &&
	       !memcmp(&ambient, &rhs.ambient, sizeof(NJS_VECTOR));
}

bool StageLight::operator!=(const StageLight& rhs) const
{
	return !(*this == rhs);
}

bool StageLights::operator==(const StageLights& rhs) const
{
	return lights[0] == rhs.lights[0] &&
	       lights[1] == rhs.lights[1] &&
	       lights[2] == rhs.lights[2] &&
	       lights[3] == rhs.lights[3];
}

bool StageLights::operator!=(const StageLights& rhs) const
{
	return !(*this == rhs);
}

static bool use_time(Uint32 level, Uint32 act)
{
	if (level < LevelIDs_StationSquare || (level >= LevelIDs_Past && level <= LevelIDs_SandHill))
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

		// Chao Race should not be adjusted. It always takes
		// place during the day.
		case LevelIDs_ChaoRace:
		{
			return false;
		}
	}
}

// private static
bool  LanternInstance::use_palette_           = false;
float LanternInstance::diffuse_blend_factor_  = 0.0f;
float LanternInstance::specular_blend_factor_ = 0.0f;

// public static
bool LanternInstance::diffuse_override          = false;
bool LanternInstance::diffuse_override_is_temp  = false;
bool LanternInstance::specular_override         = false;
bool LanternInstance::specular_override_is_temp = false;

bool LanternInstance::use_palette()
{
	return use_palette_;
}

float LanternInstance::diffuse_blend_factor()
{
	return diffuse_blend_factor_;
}

float LanternInstance::specular_blend_factor()
{
	return specular_blend_factor_;
}

inline bool is_ingame()
{
	switch (static_cast<GameModes>(GameMode))
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
std::string LanternInstance::palette_id(Sint32 level, Sint32 act)
{
	if (use_time(level, act))
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
			act = static_cast<Sint32>(GetTimeOfDay());
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
				if (is_ingame())
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

	const int n = (level - 10) / 26;

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

	const auto i = static_cast<char>(level - (10 + 26 * n) + 'A');
	result << i << act;
	return result.str();
}

LanternInstance::LanternInstance(ShaderParameter<Texture>* atlas)
	: atlas_(atlas)
{
}

LanternInstance::LanternInstance(LanternInstance&& other) noexcept
	: atlas_(std::exchange(other.atlas_, nullptr)),
	  source_lights_(other.source_lights_),
	  sl_direction_(other.sl_direction_),
	  diffuse_index_(std::exchange(other.diffuse_index_, -1)),
	  specular_index_(std::exchange(other.specular_index_, -1)),
	  last_time_(std::exchange(other.last_time_, -1)),
	  last_act_(std::exchange(other.last_act_, -1)),
	  last_level_(std::exchange(other.last_level_, -1))
{
}

LanternInstance& LanternInstance::operator=(LanternInstance&& rhs) noexcept
{
	if (&rhs != this)
	{
		atlas_          = std::exchange(rhs.atlas_, nullptr);
		source_lights_  = rhs.source_lights_;
		sl_direction_   = rhs.sl_direction_;
		diffuse_index_  = std::exchange(rhs.diffuse_index_, -1);
		specular_index_ = std::exchange(rhs.specular_index_, -1);
		last_time_      = std::exchange(rhs.last_time_, -1);
		last_act_       = std::exchange(rhs.last_act_, -1);
		last_level_     = std::exchange(rhs.last_level_, -1);
	}

	return *this;
}

LanternInstance::~LanternInstance()
{
	if (atlas_ != nullptr)
	{
		*atlas_ = nullptr;
	}
}

void LanternInstance::set_last_level(Sint32 level, Sint32 act)
{
	last_level_ = level;
	last_act_   = act;
}

/// <summary>
/// Loads palette parameter data (light direction) from the specified path.
/// </summary>
/// <param name="path">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::load_source(const std::string& path)
{
	auto file = std::ifstream(path, std::ios::binary);

	if (!file.is_open())
	{
		PrintDebug("[lantern] Lantern source not found: %s\n", path.c_str());
		sl_direction_ = { 0.0f, -1.0f, 0.0f };
		return false;
	}

	PrintDebug("[lantern] Loading lantern source: %s\n", path.c_str());

	for (auto& source_light : source_lights_)
	{
		file.read(reinterpret_cast<char*>(&source_light), sizeof(SourceLight));
	}

	file.close();

	NJS_MATRIX m;
	const SourceLight& last_source_light = source_lights_[15];

	njUnitMatrix(m);
	njRotateY(m, last_source_light.stage.y);
	njRotateZ(m, last_source_light.stage.z);

	// Default light direction is down, so we want to rotate relative to that.
	NJS_VECTOR vs = { 0.0f, -1.0f, 0.0f };
	njCalcVector(m, &vs, &sl_direction_);
	param::LightDirection = -*reinterpret_cast<D3DXVECTOR3*>(&sl_direction_);

	PrintDebug("[lantern] Source light rotation (direction): y: %d, z: %d (x: %f, y: %f, z: %f)\n",
	           last_source_light.stage.y, last_source_light.stage.z,
	           sl_direction_.x, sl_direction_.y, sl_direction_.z);

	return true;
}

/// <summary>
/// Loads palette parameter data (light direction) for the specified stage/act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::load_source(Sint32 level, Sint32 act)
{
	std::stringstream name;
	name << "SL" << palette_id(level, act) << "B.BIN";
	return load_source(globals::get_system_path(name.str()));
}

/// <summary>
/// Loads palette data from the specified path.
/// </summary>
/// <param name="path">Path to the file.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::load_palette(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open())
	{
		PrintDebug("[lantern] Lantern palette not found: %s\n", path.c_str());
		return false;
	}

	PrintDebug("[lantern] Loading lantern palette: %s\n", path.c_str());

	std::vector<ColorPair> color_data;

	do
	{
		ColorPair pair = {};
		file.read(reinterpret_cast<char*>(&pair.diffuse), sizeof(NJS_COLOR));
		file.read(reinterpret_cast<char*>(&pair.specular), sizeof(NJS_COLOR));
		color_data.push_back(pair);
		file.peek();
	} while (!file.eof());

	file.close();

	if (color_data.size() > palette_pairs_max)
	{
		PrintDebug("[lantern] WARNING: Palette size %u exceeds standard maximum of %u.\n", color_data.size(), palette_pairs_max);
	}

	memset(LSPAL.data(), 0, sizeof(ColorPair) * palette_pairs_max);
	memcpy(LSPAL.data(),
		color_data.data(),
		std::min(sizeof(ColorPair) * color_data.size(), sizeof(ColorPair) * palette_pairs_max));
	generate_atlas();
	return true;
}

void LanternInstance::palette_from_rgb(int index, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	for (size_t x = 0; x < palette_index_length; x++)
	{
		if (specular)
		{
			LSPAL[index][x].specular.argb.a = 255;
			LSPAL[index][x].specular.argb.r = r;
			LSPAL[index][x].specular.argb.g = g;
			LSPAL[index][x].specular.argb.b = b;
		}
		else
		{
			LSPAL[index][x].diffuse.argb.a = 255;
			LSPAL[index][x].diffuse.argb.r = r;
			LSPAL[index][x].diffuse.argb.g = g;
			LSPAL[index][x].diffuse.argb.b = b;
		}
	}

	if (apply)
	{
		generate_atlas();
	}
}

void LanternInstance::palette_from_array(int index, const NJS_ARGB* colors, bool specular, bool apply)
{
	for (size_t x = 0; x < palette_index_length; x++)
	{
		if (specular)
		{
			LSPAL[index][x].specular.argb.a = static_cast<Uint8>(colors[x].a);
			LSPAL[index][x].specular.argb.r = static_cast<Uint8>(colors[x].r);
			LSPAL[index][x].specular.argb.g = static_cast<Uint8>(colors[x].g);
			LSPAL[index][x].specular.argb.b = static_cast<Uint8>(colors[x].b);
		}
		else
		{
			LSPAL[index][x].diffuse.argb.a = static_cast<Uint8>(colors[x].a);
			LSPAL[index][x].diffuse.argb.r = static_cast<Uint8>(colors[x].r);
			LSPAL[index][x].diffuse.argb.g = static_cast<Uint8>(colors[x].g);
			LSPAL[index][x].diffuse.argb.b = static_cast<Uint8>(colors[x].b);
		}
	}

	if (apply)
	{
		generate_atlas();
	}
}

void LanternInstance::palette_from_mix(int index, int index_source, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	for (size_t x = 0; x < palette_index_length; x++)
	{
		if (specular)
		{
			NJS_BGRA& color = LSPAL[index][x].specular.argb;

			color.a = 255;
			color.r = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].specular.argb.r + r));
			color.g = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].specular.argb.g + g));
			color.b = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].specular.argb.b + b));
		}
		else
		{
			NJS_BGRA& color = LSPAL[index][x].diffuse.argb;

			color.a = 255;
			color.r = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].diffuse.argb.r + r));
			color.g = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].diffuse.argb.g + g));
			color.b = static_cast<Uint8>(std::min<Uint32>(255, LSPAL[index_source][x].diffuse.argb.b + b));
		}
	}

	if (apply)
	{
		generate_atlas();
	}
}

void LanternInstance::generate_atlas()
{
	const bool is_32bit = d3d::supports_xrgb();
	Texture texture = atlas_->value();

	if (texture == nullptr)
	{
		// Using a floating point texture to support the GeForce 6000 series cards.
		const auto format = is_32bit ? D3DFMT_X8R8G8B8 : D3DFMT_A32B32G32R32F;

		if (FAILED(d3d::device->CreateTexture(palette_index_length, 2 * palette_index_count, 1, 0, format, D3DPOOL_MANAGED, &texture, nullptr)))
		{
			throw std::exception("Failed to create palette texture!");
		}

		*atlas_ = texture;
	}
	else
	{
		// Release all of its references in case there are lingering textures.
		atlas_->release();
		*atlas_ = texture;
	}

	D3DLOCKED_RECT rect;
	if (FAILED(texture->LockRect(0, &rect, nullptr, 0)))
	{
		throw std::exception("Failed to lock texture rect!");
	}

	struct ABGR32F
	{
		float r, g, b, a;
	};

	static_assert(sizeof(ABGR32F) == sizeof(float) * 4, "nope");

	for (size_t i = 0; i < palette_index_count; i++)
	{
		const auto index = i * palette_index_length;

		/*if (index >= palette_pairs_.size() || index + palette_index_length >= palette_pairs_.size())
		{
			break;
		}*/

		// Multiplied by 2 since colors are stored in pairs of diffuse and specular.
		const auto y = 2 * palette_index_length * i;

		if (is_32bit)
		{
			const auto pixels = static_cast<NJS_COLOR*>(rect.pBits);

			for (size_t x = 0; x < palette_index_length; x++)
			{
				const auto& color = LSPAL[i][x];

				auto& diffuse  = pixels[y + x];
				auto& specular = pixels[palette_index_length + y + x];

				diffuse  = color.diffuse;
				specular = color.specular;
			}
		}
		else
		{
			const auto pixels = static_cast<ABGR32F*>(rect.pBits);

			for (size_t x = 0; x < palette_index_length; x++)
			{

				auto& diffuse  = pixels[y + x];
				auto& specular = pixels[palette_index_length + y + x];

				const auto& diffuse_argb = LSPAL[i][x].diffuse.argb;

				diffuse.r = static_cast<float>(diffuse_argb.r) / 255.0f;
				diffuse.g = static_cast<float>(diffuse_argb.g) / 255.0f;
				diffuse.b = static_cast<float>(diffuse_argb.b) / 255.0f;
				diffuse.a = static_cast<float>(diffuse_argb.a) / 255.0f;

				const auto& specular_argb = LSPAL[i][x].specular.argb;

				specular.r = static_cast<float>(specular_argb.r) / 255.0f;
				specular.g = static_cast<float>(specular_argb.g) / 255.0f;
				specular.b = static_cast<float>(specular_argb.b) / 255.0f;
				specular.a = static_cast<float>(specular_argb.a) / 255.0f;
			}
		}
	}

	texture->UnlockRect(0);
}

/// <summary>
/// Loads palette data for the specified stage and act.
/// </summary>
/// <param name="level">Current level/stage.</param>
/// <param name="act">Current act.</param>
/// <returns><c>true</c> on success.</returns>
bool LanternInstance::load_palette(Sint32 level, Sint32 act)
{
	std::stringstream name;
	name << "PL" << palette_id(level, act) << "B.BIN";
	return load_palette(globals::get_system_path(name.str()));
}

void LanternInstance::diffuse_blend_factor(float f)
{
	auto value = param::BlendFactor.value();
	value.x = f;
	param::BlendFactor = value;

	diffuse_blend_factor_ = f;
}

void LanternInstance::specular_blend_factor(float f)
{
	auto value = param::BlendFactor.value();
	value.y = f;
	param::BlendFactor = value;

	specular_blend_factor_ = f;
}

/// <summary>
/// Selects a diffuse and specular palette index based on the given SADX light type and material flags.
/// </summary>
/// <param name="type">SADX light type.</param>
/// <param name="flags">Material flags.</param>
void LanternInstance::set_palettes(Sint32 type, Uint32 flags)
{
#ifdef _DEBUG
	auto pad = ControllerPointers[0];

	if (pad && pad->HeldButtons & Buttons_Z)
	{
		use_palette_ = false;
		d3d::do_effect = false;
		return;
	}
#endif

	// SA1 light types
	// 0: Default (diffuse 0 specular 0/1)
	// 2: Most NPCs and cutscene objects (diffuse 2 specular 2/3)
	// 4: Chaos 2, Chaos 6, Perfect Chaos (diffuse 4 specular 4/5)

	// SADX light types:
	// 0: Default
	// 2: Most NPCs and cutscene objects
	// 4: Super Sonic, Gamma, Gamma NPC (Egg Carrier inside), Question Mark (because of Gamma's)
	// 6: Kiki, Boa - Boa, Rhinotank, Sweep, Spinner, Egg Keeper, Zero glove attack (gameplay, cutscenes), Perfect Chaos, Egg Hornet, Egg Walker, Egg Viper (cutscene only), Zero boss, E - 101, E - 101R

	Sint32 diffuse  = -1;
	Sint32 specular = -1;

	globals::light_type = type;
	const bool ignore_specular = !!(flags & NJD_FLAG_IGNORE_SPECULAR);

	switch (type)
	{
		case 0:
		case 6:
			diffuse  = 0;
			specular = ignore_specular ? 0 : 1;

			globals::debug_stage_light_dir = *reinterpret_cast<NJS_VECTOR*>(&Direct3D_CurrentLight.Direction);
			break;

		case 2:
			diffuse  = 2;
			specular = ignore_specular ? 2 : 3;
			break;

		case 4:
			diffuse = 4;
			specular = ignore_specular ? 4 : 5;
			break;

		default:
			break;
	}

	if (!diffuse_override)
	{
		diffuse_index(diffuse);
	}

	if (!specular_override)
	{
		specular_index(specular);
	}

	d3d::do_effect = use_palette_ = diffuse > -1 && specular > -1;
}

void LanternInstance::diffuse_index(Sint32 value)
{
	diffuse_index_ = value;
}

void LanternInstance::specular_index(Sint32 value)
{
	specular_index_ = value;
}

Sint32 LanternInstance::diffuse_index()
{
	return diffuse_index_;
}

Sint32 LanternInstance::specular_index()
{
	return specular_index_;
}

void LanternInstance::light_direction(const NJS_VECTOR& d)
{
	sl_direction_ = d;
}

const NJS_VECTOR& LanternInstance::light_direction()
{
	return sl_direction_;
}

// Collection

void LanternCollection::set_last_level(Sint32 level, Sint32 act)
{
	for (auto& i : instances_)
	{
		i.set_last_level(level, act);
	}
}

bool LanternCollection::load_palette(Sint32 level, Sint32 act)
{
	size_t count = 0;

	for (auto& i : instances_)
	{
		if (i.load_palette(level, act))
		{
			++count;
		}
	}

	return count == instances_.size();
}

bool LanternCollection::load_palette(const std::string& path)
{
	size_t count = 0;

	for (auto& i : instances_)
	{
		if (i.load_palette(path))
		{
			++count;
		}
	}

	return count == instances_.size();
}

bool LanternCollection::load_source(Sint32 level, Sint32 act)
{
	size_t count = 0;

	for (auto& i : instances_)
	{
		if (i.load_source(level, act))
		{
			++count;
		}
	}

	return count == instances_.size();
}

bool LanternCollection::load_source(const std::string& path)
{
	size_t count = 0;

	for (auto& i : instances_)
	{
		if (i.load_source(path))
		{
			++count;
		}
	}

	return count == instances_.size();
}

bool LanternCollection::run_pl_callbacks(Sint32 level, Sint32 act, Sint8 time)
{
	bool result = false;

	for (const auto& callback : pl_callbacks_)
	{
		const auto path_ptr = callback(level, act);

		if (path_ptr == nullptr)
		{
			continue;
		}

		std::string path_str = path_ptr;

		for (auto& instance : instances_)
		{
			if (level == instance.last_level_ &&
			    act == instance.last_act_ &&
			    time == instance.last_time_)
			{
				result = true;
				break;
			}

			if (!instance.load_palette(path_str))
			{
				return false;
			}

			instance.last_time_  = time;
			instance.last_level_ = level;
			instance.last_act_   = act;

			result = true;
		}

		if (result)
		{
			break;
		}
	}

	return result;
}

bool LanternCollection::run_sl_callbacks(Sint32 level, Sint32 act, Sint8 time)
{
	bool result = false;

	for (const auto& callback : sl_callbacks_)
	{
		const char* path_ptr = callback(level, act);

		if (path_ptr == nullptr)
		{
			continue;
		}

		const std::string path_str = path_ptr;

		for (auto& instance : instances_)
		{
			if (level == instance.last_level_ &&
			    act == instance.last_act_ &&
			    time == instance.last_time_)
			{
				result = true;
				break;
			}

			if (!instance.load_source(path_str))
			{
				return false;
			}

			instance.last_time_  = time;
			instance.last_level_ = level;
			instance.last_act_   = act;

			result = true;
		}

		if (result)
		{
			break;
		}
	}

	return result;
}

bool LanternCollection::load_files()
{
	const auto time = GetTimeOfDay();
	size_t count = 0;

	if (instances_.empty())
	{
		instances_.emplace_back(&param::PaletteA);
	}

	const bool pl_handled = run_pl_callbacks(CurrentLevel, CurrentAct, time);
	const bool sl_handled = run_sl_callbacks(CurrentLevel, CurrentAct, time);

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

	for (auto& instance : instances_)
	{
		// This is a fallback for cases where stage acts
		// palettes from the previous acts.
		for (int i = CurrentAct; i >= 0; i--)
		{
			int level = CurrentLevel;
			int act   = i;

			if (use_time(level, act))
			{
				GetTimeOfDayLevelAndAct(&level, &act);
			}

			if (!pl_handled && !sl_handled &&
			    level == instance.last_level_ &&
			    act == instance.last_act_ &&
			    time == instance.last_time_)
			{
				break;
			}

			if (!pl_handled && !instance.load_palette(CurrentLevel, i))
			{
				// Palette loading is critical for lighting, so
				// continue immediately on failure.
				continue;
			}

			if (!sl_handled)
			{
				// Source light loading on the other hand is not
				// a requirement, so failure is fine.
				instance.load_source(CurrentLevel, i);
			}

			instance.last_time_  = time;
			instance.last_level_ = level;
			instance.last_act_   = act;

			LanternInstance::use_palette_ = false;
			d3d::do_effect = false;
			++count;
			break;
		}
	}

	return count == instances_.size();
}

void LanternCollection::set_palettes(Sint32 type, Uint32 flags)
{
	for (auto& i : instances_)
	{
		i.set_palettes(type, flags);
	}
}

void LanternCollection::diffuse_index(Sint32 value)
{
	for (auto& i : instances_)
	{
		i.diffuse_index(value);
	}
}

void LanternCollection::specular_index(Sint32 value)
{
	for (auto& i : instances_)
	{
		i.specular_index(value);
	}
}

Sint32 LanternCollection::diffuse_index()
{
	return -1;
}

Sint32 LanternCollection::specular_index()
{
	return -1;
}

void LanternCollection::light_direction(const NJS_VECTOR& d)
{
	for (auto& i : instances_)
	{
		i.light_direction(d);
	}
}

const NJS_VECTOR& LanternCollection::light_direction()
{
	return instances_[0].light_direction();
}

void LanternCollection::forward_blend_all(bool enable)
{
	for (size_t i = 0; i < LanternInstance::palette_index_count; i++)
	{
		const auto n = enable ? static_cast<Sint32>(i) : -1;
		diffuse_blend_[i]  = n;
		specular_blend_[i] = n;
	}
}

inline void blend_all(std::array<Sint32, LanternInstance::palette_index_count>& src_array, Sint32 value)
{
	for (auto& i : src_array)
	{
		i = value;
	}
}

void LanternCollection::diffuse_blend_all(int value)
{
	blend_all(diffuse_blend_, value);
}

void LanternCollection::specular_blend_all(int value)
{
	blend_all(specular_blend_, value);
}

void LanternCollection::diffuse_blend(int index, int value)
{
	diffuse_blend_[index] = value;
}

void LanternCollection::specular_blend(int index, int value)
{
	specular_blend_[index] = value;
}

int LanternCollection::diffuse_blend(int index) const
{
	return diffuse_blend_[index];
}

int LanternCollection::specular_blend(int index) const
{
	return specular_blend_[index];
}

inline float index_float(Sint32 i, Sint32 offset)
{
	return (static_cast<float>(2 * i + offset) + 0.5f) / static_cast<float>(2 * LanternInstance::palette_index_count);
}

void LanternCollection::apply_parameters()
{
	if (instances_.empty())
	{
		return;
	}

	// .xy is diffuse A and B, .zw is specular A and B.
	D3DXVECTOR4 indices { 0.0f, 0.0f, 0.0f, 0.0f };

	// .x is diffuse, .y is specular.
	D3DXVECTOR2 blend_factors { 0.0f, 0.0f };

	LanternInstance& i = instances_[0];

	const int d = i.diffuse_index();

	if (d >= 0)
	{
		indices.x = index_float(d, 0);

		if (diffuse_blend_[d] >= 0)
		{
			indices.y       = index_float(diffuse_blend_[d], 0);
			blend_factors.x = LanternInstance::diffuse_blend_factor_;
		}
	}

	const int s = i.specular_index();

	if (s >= 0)
	{
		indices.z = index_float(s, 1);

		if (specular_blend_[s] >= 0)
		{
			indices.w       = index_float(specular_blend_[s], 1);
			blend_factors.y = LanternInstance::specular_blend_factor_;
		}
	}

	param::Indices     = indices;
	param::BlendFactor = blend_factors;
}

LanternInstance& LanternCollection::operator[](size_t i)
{
	return instances_[i];
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
	c.erase(std::remove(c.begin(), c.end(), callback), c.end());
}

size_t LanternCollection::add(LanternInstance& src)
{
	instances_.emplace_back(std::move(src));
	return instances_.size() - 1;
}

void LanternCollection::remove(size_t index)
{
	instances_.erase(instances_.begin() + index);
}

size_t LanternCollection::size() const
{
	return instances_.size();
}

void LanternCollection::add_pl_callback(lantern_load_cb callback)
{
	callback_add(pl_callbacks_, callback);
}

void LanternCollection::palette_from_rgb(int index, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	LanternInstance& i = instances_[0];
	i.palette_from_rgb(index, r, g, b, specular, apply);
}

void LanternCollection::palette_from_array(int index, const NJS_ARGB* colors, bool specular, bool apply)
{
	LanternInstance& i = instances_[0];
	i.palette_from_array(index, colors, specular, apply);
}

void LanternCollection::palette_from_mix(int index, int index_source, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	LanternInstance& i = instances_[0];
	i.palette_from_mix(index, index_source, r, g, b, specular, apply);
}

void LanternCollection::generate_atlas()
{
	LanternInstance& i = instances_[0];
	i.generate_atlas();
}

void LanternCollection::remove_pl_callback(lantern_load_cb callback)
{
	callback_del(pl_callbacks_, callback);
}

void LanternCollection::add_sl_callback(lantern_load_cb callback)
{
	callback_add(sl_callbacks_, callback);
}

void LanternCollection::remove_sl_callback(lantern_load_cb callback)
{
	callback_del(sl_callbacks_, callback);
}
