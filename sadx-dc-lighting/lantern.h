#pragma once

#include <string>
#include <ninja.h>
#include <d3d9.h>
#include <deque>
#include <SADXStructs.h>

#include "EffectParameter.h"
#include "../include/lanternapi.h"

#pragma pack(push, 1)
struct SourceLight_t
{
	Angle y, z;

	float ambient[3];
	float unknown[2];
	float ambient_power;

	bool operator==(const SourceLight_t& rhs) const;
	bool operator!=(const SourceLight_t& rhs) const;
};

union SourceLight
{
	SourceLight_t stage;
	PaletteLight source;
};
#pragma pack(pop)

static_assert(sizeof(SourceLight) == 0x60, "SourceLight size mismatch");
template<> bool EffectParameter<SourceLight_t>::Commit(Effect effect);

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
	virtual void SetDiffuse(Sint32 n) = 0;
	virtual void SetSpecular(Sint32 n) = 0;
	virtual Sint32 GetDiffuse() = 0;
	virtual Sint32 GetSpecular() = 0;
	virtual void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) = 0;
};

class LanternInstance : ILantern
{
	EffectParameter<Texture>* atlas;
	EffectParameter<float>* diffuse_param;
	EffectParameter<float>* specular_param;

	void copy(LanternInstance& inst);

public:
	LanternInstance(EffectParameter<Texture>* atlas, EffectParameter<float>* diffuse_param, EffectParameter<float>* specular_param);
	LanternInstance(LanternInstance&& instance) noexcept;

	LanternInstance(const LanternInstance&) = default;
	LanternInstance& operator=(LanternInstance&&) noexcept;

	~LanternInstance();

	static bool diffuse_override;
	static bool specular_override;
	static float blend_factor;
	static bool use_palette;

	Sint32 blend_type       = -1;
	Sint8 last_time         = -1;
	Sint32 last_act         = -1;
	Sint32 last_level       = -1;
	Sint32 diffuse_index    = -1;
	Sint32 specular_index   = -1;
	Sint32 diffuse_index_b  = -1;
	Sint32 specular_index_b = -1;

	static bool UsePalette();
	static float BlendFactor();
	static std::string PaletteId(Sint32 level, Sint32 act);
	static void SetBlendFactor(float f);

	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void SetDiffuse(Sint32 n) override;
	void SetSpecular(Sint32 n) override;
	Sint32 GetDiffuse() override;
	Sint32 GetSpecular() override;
	void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) override;

	void SetDiffuseB(Sint32 n);
	void SetSpecularB(Sint32 n);
	Sint32 GetDiffuseB() const;
	Sint32 GetSpecularB() const;
};

class LanternCollection : ILantern
{
	std::deque<LanternInstance> instances;
	std::deque<lantern_load_t> pl_callbacks;
	std::deque<lantern_load_t> sl_callbacks;

	static void callback_add(std::deque<lantern_load_t>& c, lantern_load_t callback);
	static void callback_del(std::deque<lantern_load_t>& c, lantern_load_t callback);

public:
	size_t Add(LanternInstance& src);
	void Remove(size_t index);

	auto Size() const
	{
		return instances.size();
	}

	void AddPlCallback(lantern_load_t callback);
	void RemovePlCallback(lantern_load_t callback);
	void AddSlCallback(lantern_load_t callback);
	void RemoveSlCallback(lantern_load_t callback);
	bool RunPlCallbacks(Sint32 level, Sint32 act, Sint8 time);
	bool RunSlCallbacks(Sint32 level, Sint32 act, Sint8 time);
	bool LoadFiles();

	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void SetDiffuse(Sint32 n) override;
	void SetSpecular(Sint32 n) override;

	Sint32 GetDiffuse() override
	{
		return -1;
	}

	Sint32 GetSpecular() override
	{
		return -1;
	}

	void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) override;

	LanternInstance& operator[](size_t i) { return instances[i]; }
};

void UpdateLightDirections(const NJS_VECTOR& dir);
