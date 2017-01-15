#pragma once

#include <string>
#include <ninja.h>
#include <d3d9.h>
#include <deque>
#include <SADXModLoader/SADXStructs.h>

#include "EffectParameter.h"

struct LanternPalette
{
	// The first set of colors in the pair.
	IDirect3DTexture9* diffuse;
	// The second set of colors in the pair.
	IDirect3DTexture9* specular;
};

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
template<> void EffectParameter<SourceLight_t>::Commit();

class LanternInstance;

class ILantern
{
public:
	virtual ~ILantern() = default;

	virtual bool LoadFiles() = 0;
	virtual bool LoadPalette(Sint32 level, Sint32 act) = 0;
	virtual bool LoadPalette(const std::string& path) = 0;
	virtual bool LoadSource(Sint32 level, Sint32 act) = 0;
	virtual bool LoadSource(const std::string& path) = 0;
	virtual void SetLastLevel(Sint32 level, Sint32 act) = 0;
	virtual void SetPalettes(Sint32 type, Uint32 flags) = 0;
	virtual void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) = 0;
};

class LanternInstance : ILantern
{
	static float blend_factor;
	static bool use_palette;

	EffectParameter<IDirect3DTexture9*>* diffuse_param;
	EffectParameter<IDirect3DTexture9*>* specular_param;
	LanternPalette palette[8];

	Sint32 blend_type;
	Sint8 last_time;
	Sint32 last_act;
	Sint32 last_level;

	void set_diffuse(Sint32 diffuse) const;
	void set_specular(Sint32 specular) const;

public:
	LanternInstance(EffectParameter<IDirect3DTexture9*>* diffuse, EffectParameter<IDirect3DTexture9*>* specular);
	LanternInstance(LanternInstance&& instance) noexcept;

	LanternInstance(const LanternInstance&) = default;
	LanternInstance& operator=(LanternInstance&&) noexcept;

	~LanternInstance();

	static bool UsePalette();
	static std::string PaletteId(Sint32 level, Sint32 act);
	static bool LoadFiles(LanternInstance& instance);
	static void SetBlendFactor(float f);

	bool LoadFiles() override;
	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) override;
};

class LanternCollection : ILantern
{
	std::deque<LanternInstance> instances;

public:
	size_t Add(LanternInstance& src);
	void Remove(size_t index);
	auto Size() { return instances.size(); }

	bool LoadFiles() override;
	bool LoadPalette(Sint32 level, Sint32 act) override;
	bool LoadPalette(const std::string& path) override;
	bool LoadSource(Sint32 level, Sint32 act) override;
	bool LoadSource(const std::string& path) override;
	void SetLastLevel(Sint32 level, Sint32 act) override;
	void SetPalettes(Sint32 type, Uint32 flags) override;
	void SetSelfBlend(Sint32 type, Sint32 diffuse, Sint32 specular) override;

	LanternInstance& operator[](size_t i) { return instances[i]; }
};

void UpdateLightDirections(const NJS_VECTOR& dir);
