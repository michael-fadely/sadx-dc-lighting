#pragma once

#include <ninja.h>
#include <array>
#include <deque>
#include <string>
#include <SADXStructs.h>

#include "ShaderParameter.h"
#include "../include/lanternapi.h"

#pragma pack(push, 1)

// RY RZ R G B SP DI AM
struct SourceLight_t
{
	Angle y, z;

	float color[3];
	float specular;
	float diffuse;
	float ambient;
	float unknown2[15];

	bool operator==(const SourceLight_t& rhs) const;
	bool operator!=(const SourceLight_t& rhs) const;
};

union SourceLight
{
	SourceLight_t stage;
	PaletteLight source;
};

struct StageLight
{
	NJS_VECTOR direction;
	float specular;
	float multiplier;
	NJS_VECTOR diffuse;
	NJS_VECTOR ambient;
	float padding[5];

	bool operator==(const StageLight& rhs) const;
	bool operator!=(const StageLight& rhs) const;
};

struct StageLights
{
	StageLight lights[4] {};

	bool operator==(const StageLights& rhs) const;
	bool operator!=(const StageLights& rhs) const;
};

struct ColorPair
{
	NJS_COLOR diffuse, specular;
};

#pragma pack(pop)

static_assert(sizeof(SourceLight) == 0x60, "SourceLight size mismatch");

class LanternCollection;

class LanternInstance
{
	friend class LanternCollection;

public:
	/**
	 * \brief The maximum number of palette indices.
	 */
	static constexpr size_t palette_index_count = 8;

	/**
	 * \brief The number of diffuse/specular pairs in a given palette index.
	 */
	static constexpr size_t palette_index_length = 256;

	/**
	 * \brief The maximum number of color pairs in a PL file.
	 */
	static constexpr size_t palette_pairs_max = palette_index_count * palette_index_length;

	/**
	 * \brief The maximum number of source lights.
	 */
	static constexpr size_t source_light_count = 16;

private:
	// TODO: handle externally
	ShaderParameter<Texture>* atlas_;
	std::array<SourceLight, source_light_count> source_lights_ {};
	NJS_VECTOR sl_direction_ {};

	Sint32 diffuse_index_ = -1;
	Sint32 specular_index_ = -1;

	Sint8  last_pl_time_ = -1;
	Sint8  last_sl_time_ = -1;
	Sint32 last_pl_act_ = -1;
	Sint32 last_sl_act_ = -1;
	Sint32 last_pl_level_ = -1;
	Sint32 last_sl_level_ = -1;

	static bool use_palette_;
	static float diffuse_blend_factor_;
	static float specular_blend_factor_;

public:
	explicit LanternInstance(ShaderParameter<Texture>* atlas);
	LanternInstance(LanternInstance&& other) noexcept;
	LanternInstance(const LanternInstance&) = delete;

	LanternInstance& operator=(LanternInstance&) = delete;
	LanternInstance& operator=(LanternInstance&&) noexcept;

	~LanternInstance();

	static bool diffuse_override;
	static bool diffuse_override_is_temp;
	static bool specular_override;
	static bool specular_override_is_temp;

	static bool use_palette();
	static float diffuse_blend_factor();
	static float specular_blend_factor();
	static void diffuse_blend_factor(float f);
	static void specular_blend_factor(float f);

	static std::string palette_id(Sint32 level, Sint32 act);

	bool load_palette(Sint32 level, Sint32 act);
	bool load_palette(const std::string& path);
	void palette_from_rgb(int index, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply);
	void palette_from_array(int index, const NJS_ARGB* colors, bool specular, bool apply);
	void palette_from_mix(int index, int index_source, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply);
	void generate_atlas();
	bool load_source(Sint32 level, Sint32 act);
	bool load_source(const std::string& path);
	void set_last_level(Sint32 level, Sint32 act);
	void set_palettes(Sint32 type, Uint32 flags);
	void diffuse_index(Sint32 value);
	void specular_index(Sint32 value);
	Sint32 diffuse_index() const;
	Sint32 specular_index() const;
	void light_direction(const NJS_VECTOR& d);
	const NJS_VECTOR& light_direction() const;
};

class LanternCollection
{
	std::deque<LanternInstance> instances_;
	std::deque<lantern_load_cb> pl_callbacks_;
	std::deque<lantern_load_cb> sl_callbacks_;

	static void callback_add(std::deque<lantern_load_cb>& c, lantern_load_cb callback);
	static void callback_del(std::deque<lantern_load_cb>& c, lantern_load_cb callback);

	std::array<Sint32, LanternInstance::palette_index_count> diffuse_blend_  = { -1, -1, -1, -1, -1, -1, -1, -1 };
	std::array<Sint32, LanternInstance::palette_index_count> specular_blend_ = { -1, -1, -1, -1, -1, -1, -1, -1 };

public:
	size_t add(LanternInstance& src);
	void remove(size_t index);
	size_t size() const;

	void palette_from_rgb(int index, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply);
	void palette_from_array(int index, const NJS_ARGB* colors, bool specular, bool apply);
	void palette_from_mix(int index, int index_source, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply);
	void generate_atlas();
	void add_pl_callback(lantern_load_cb callback);
	void remove_pl_callback(lantern_load_cb callback);
	void add_sl_callback(lantern_load_cb callback);
	void remove_sl_callback(lantern_load_cb callback);
	bool run_pl_callbacks(Sint32 level, Sint32 act, Sint8 time);
	bool run_sl_callbacks(Sint32 level, Sint32 act, Sint8 time);
	bool load_files();

	// TODO: Expose to API when explicit multi-palette management is implemented.
	// TODO: Rewrite and reformat documentation below.
	/// Blend all indices of diffuse and specular to the same index
	/// of a secondary palette atlas.
	void forward_blend_all(bool enable);
	/// Blend all diffuse indices to a single destination index.
	void diffuse_blend_all(int value);
	/// Blend all specular indices to a single destination index.
	void specular_blend_all(int value);
	/// Blend a single diffuse index to a single destination index.
	void diffuse_blend(int index, int value);
	/// Get current diffuse blend destination index.
	int diffuse_blend(int index) const;
	/// Blend a single specular index to a single destination index.
	void specular_blend(int index, int value);
	/// Get current specular blend destination index.
	int specular_blend(int index) const;
	/// Apply necessary shader parameters.
	void apply_parameters();

	LanternInstance& operator[](size_t i);

	bool load_palette(Sint32 level, Sint32 act);
	bool load_palette(const std::string& path);
	bool load_source(Sint32 level, Sint32 act);
	bool load_source(const std::string& path);
	void set_last_level(Sint32 level, Sint32 act);
	void set_palettes(Sint32 type, Uint32 flags);
	void diffuse_index(Sint32 value);
	void specular_index(Sint32 value);
	void light_direction(const NJS_VECTOR& d);
	const NJS_VECTOR& light_direction() const;
};
