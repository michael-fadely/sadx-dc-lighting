#pragma once

#include <ninja.h>
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
	StageLight lights[4]{};

	bool operator==(const StageLights& rhs) const;
	bool operator!=(const StageLights& rhs) const;
};

#pragma pack(pop)

static_assert(sizeof(SourceLight) == 0x60, "SourceLight size mismatch");
template<> bool ShaderParameter<SourceLight_t>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<StageLights>::Commit(IDirect3DDevice9* device);

class LanternInstance;

class ILantern
{
public:
	virtual ~ILantern() = default;

	virtual bool LoadPalette(Sint32 level, Sint32 act) = 0;
	virtual bool LoadPalette(const std::string& path) = 0;
	virtual bool LoadSource(Sint32 level, Sint32 act) = 0;
	virtual bool LoadSource(const std::string& path) = 0;
	virtual void SetLastLevel(Sint32 level, Sint32 act) = 0;
	virtual void SetPalettes(Sint32 type, Uint32 flags) = 0;
	virtual void DiffuseIndex(Sint32 value) = 0;
	virtual void SpecularIndex(Sint32 value) = 0;
	virtual Sint32 DiffuseIndex() = 0;
	virtual Sint32 SpecularIndex() = 0;
	virtual void LightDirection(const NJS_VECTOR& d) = 0;
	virtual const NJS_VECTOR& LightDirection() = 0;
};

class LanternInstance : ILantern
{
	// TODO: handle externally
	ShaderParameter<Texture>* atlas;
	SourceLight source_lights[16] = {};
	NJS_VECTOR sl_direction = {};

	void copy(LanternInstance& inst);

public:
	explicit LanternInstance(ShaderParameter<Texture>* atlas);
	LanternInstance(LanternInstance&& instance) noexcept;

	LanternInstance(const LanternInstance&) = default;
	LanternInstance& operator=(LanternInstance&&) noexcept;

	~LanternInstance();

	static bool diffuse_override;
	static bool diffuse_override_temp;
	static bool specular_override;
	static bool specular_override_temp;
	static float diffuse_blend_factor;
	static float specular_blend_factor;
	static bool use_palette;

	Sint8 last_time       = -1;
	Sint32 last_act       = -1;
	Sint32 last_level     = -1;
	Sint32 diffuse_index  = -1;
	Sint32 specular_index = -1;

	static bool UsePalette();
	static float DiffuseBlendFactor();
	static float SpecularBlendFactor();
	static void DiffuseBlendFactor(float f);
	static void SpecularBlendFactor(float f);

	static std::string PaletteId(Sint32 level, Sint32 act);

	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void DiffuseIndex(Sint32 value) override;
	void SpecularIndex(Sint32 value) override;
	Sint32 DiffuseIndex() override;
	Sint32 SpecularIndex() override;
	void LightDirection(const NJS_VECTOR& d) override;
	const NJS_VECTOR& LightDirection() override;
};

class LanternCollection : ILantern
{
	std::deque<LanternInstance> instances;
	std::deque<lantern_load_cb> pl_callbacks;
	std::deque<lantern_load_cb> sl_callbacks;

	static void callback_add(std::deque<lantern_load_cb>& c, lantern_load_cb callback);
	static void callback_del(std::deque<lantern_load_cb>& c, lantern_load_cb callback);

	Sint32 diffuse_blend[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	Sint32 specular_blend[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

public:
	size_t Add(LanternInstance& src);
	void Remove(size_t index);

	auto Size() const
	{
		return instances.size();
	}

	void AddPlCallback(lantern_load_cb callback);
	void RemovePlCallback(lantern_load_cb callback);
	void AddSlCallback(lantern_load_cb callback);
	void RemoveSlCallback(lantern_load_cb callback);
	bool RunPlCallbacks(Sint32 level, Sint32 act, Sint8 time);
	bool RunSlCallbacks(Sint32 level, Sint32 act, Sint8 time);
	bool LoadFiles();

	// TODO: Expose to API when explicit multi-palette management is implemented.
	/// Blend all indices of diffuse and specular to the same index
	/// of a secondary palette atlas.
	void ForwardBlendAll(bool enable);
	/// Blend all diffuse indices to a single destination index.
	void DiffuseBlendAll(int value);
	/// Blend all specular indices to a single destination index.
	void SpecularBlendAll(int value);
	/// Blend a single diffuse index to a single destination index.
	void DiffuseBlend(int index, int value);
	/// Get current diffuse blend destination index.
	int DiffuseBlend(int index) const;
	/// Blend a single specular index to a single destination index.
	void SpecularBlend(int index, int value);
	/// Get current specular blend destination index.
	int SpecularBlend(int index) const;
	/// Apply necessary shader parameters.
	void ApplyShaderParameters();

	LanternInstance& operator[](size_t i) { return instances[i]; }

	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void DiffuseIndex(Sint32 value) override;
	void SpecularIndex(Sint32 value) override;

	Sint32 DiffuseIndex() override
	{
		return -1;
	}

	Sint32 SpecularIndex() override
	{
		return -1;
	}

	void LightDirection(const NJS_VECTOR& d) override;
	const NJS_VECTOR& LightDirection() override;
};
