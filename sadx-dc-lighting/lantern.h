#pragma once

#include <string>
#include <ninja.h>
#include <d3d9.h>

#pragma pack(push, 1)
struct ColorPair
{
	NJS_COLOR diffuse, specular;
};
#pragma pack(pop)

static_assert(sizeof(ColorPair) == sizeof(NJS_COLOR) * 2, "AGAIN TRY");

struct LanternPalette
{
	// The first set of colors in the pair.
	IDirect3DTexture9* diffuse;
	// The second set of colors in the pair.
	IDirect3DTexture9* specular;
};

bool UsePalette();
std::string LanternPaletteId(Uint32 level, Uint32 act);
void UpdateLightDirections(const NJS_VECTOR& dir);
bool LoadLanternSource(Uint32 level, Uint32 act);
bool LoadLanternSource(const std::string& path);
bool LoadLanternPalette(Uint32 level, Uint32 act);
bool LoadLanternPalette(const std::string& path);
void LoadLanternFiles();
void SetPaletteLights(int type, int flags = 0);
