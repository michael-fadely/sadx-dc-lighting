#pragma once

#include <ninja.h>
#include <array>
#include <deque>
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
template<> bool ShaderParameter<SourceLight_t>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<StageLights>::commit(IDirect3DDevice9* device);

class LanternInstance;

class ILantern
{
public:
	virtual ~ILantern() = default;

	virtual bool load_palette(Sint32 level, Sint32 act) = 0;
	virtual bool load_palette(const std::string& path) = 0;
	virtual bool load_source(Sint32 level, Sint32 act) = 0;
	virtual bool load_source(const std::string& path) = 0;
	virtual void set_last_level(Sint32 level, Sint32 act) = 0;
	virtual void set_palettes(Sint32 type, Uint32 flags) = 0;
	virtual void diffuse_index(Sint32 value) = 0;
	virtual void specular_index(Sint32 value) = 0;
	virtual Sint32 diffuse_index() = 0;
	virtual Sint32 specular_index() = 0;
	virtual void light_direction(const NJS_VECTOR& d) = 0;
	virtual const NJS_VECTOR& light_direction() = 0;
};

class LanternInstance : ILantern
{
	// TODO: handle externally
	ShaderParameter<Texture>* atlas;
	std::array<ColorPair, 256 * 8> palette_pairs {};
	SourceLight source_lights[16] {};
	NJS_VECTOR sl_direction {};

	void copy(LanternInstance& inst);

public:
	explicit LanternInstance(ShaderParameter<Texture>* atlas);
	LanternInstance(LanternInstance&& inst) noexcept;

	LanternInstance(const LanternInstance&) = default;
	LanternInstance& operator=(LanternInstance&&) noexcept;

	~LanternInstance();

	static bool diffuse_override;
	static bool diffuse_override_is_temp;
	static bool specular_override;
	static bool specular_override_is_temp;
	static float diffuse_blend_factor_;
	static float specular_blend_factor_;
	static bool use_palette_;

	Sint8  last_time  = -1;
	Sint32 last_act   = -1;
	Sint32 last_level = -1;
	Sint32 diffuse_   = -1;
	Sint32 specular_  = -1;

	static bool use_palette();
	static float diffuse_blend_factor();
	static float specular_blend_factor();
	static void diffuse_blend_factor(float f);
	static void specular_blend_factor(float f);

	static std::string palette_id(Sint32 level, Sint32 act);

	bool load_palette(Sint32 level, Sint32 act) override;
	bool load_palette(const std::string& path) override;
	void generate_atlas();
	bool load_source(Sint32 level, Sint32 act) override;
	bool load_source(const std::string& path) override;
	void set_last_level(Sint32 level, Sint32 act) override;
	void set_palettes(Sint32 type, Uint32 flags) override;
	void diffuse_index(Sint32 value) override;
	void specular_index(Sint32 value) override;
	Sint32 diffuse_index() override;
	Sint32 specular_index() override;
	void light_direction(const NJS_VECTOR& d) override;
	const NJS_VECTOR& light_direction() override;
};

class LanternCollection : ILantern
{
	std::deque<LanternInstance> instances;
	std::deque<lantern_load_cb> pl_callbacks;
	std::deque<lantern_load_cb> sl_callbacks;

	static void callback_add(std::deque<lantern_load_cb>& c, lantern_load_cb callback);
	static void callback_del(std::deque<lantern_load_cb>& c, lantern_load_cb callback);

	Sint32 diffuse_blend_[8]  = { -1, -1, -1, -1, -1, -1, -1, -1 };
	Sint32 specular_blend_[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

public:
	size_t add(LanternInstance& src);
	void remove(size_t index);

	auto size() const
	{
		return instances.size();
	}

	void add_pl_callback(lantern_load_cb callback);
	void remove_pl_callback(lantern_load_cb callback);
	void add_sl_callback(lantern_load_cb callback);
	void remove_sl_callback(lantern_load_cb callback);
	bool run_pl_callbacks(Sint32 level, Sint32 act, Sint8 time);
	bool run_sl_callbacks(Sint32 level, Sint32 act, Sint8 time);
	bool load_files();

	// TODO: Expose to API when explicit multi-palette management is implemented.
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

	LanternInstance& operator[](size_t i)
	{
		return instances[i];
	}

	bool load_palette(Sint32 level, Sint32 act) override;
	bool load_palette(const std::string& path) override;
	bool load_source(Sint32 level, Sint32 act) override;
	bool load_source(const std::string& path) override;
	void set_last_level(Sint32 level, Sint32 act) override;
	void set_palettes(Sint32 type, Uint32 flags) override;
	void diffuse_index(Sint32 value) override;
	void specular_index(Sint32 value) override;

	Sint32 diffuse_index() override
	{
		return -1;
	}

	Sint32 specular_index() override
	{
		return -1;
	}

	void light_direction(const NJS_VECTOR& d) override;
	const NJS_VECTOR& light_direction() override;
};
