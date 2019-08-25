#pragma once

#include <d3d8to9.hpp>
#include <ninja.h>
#include "PaletteParameters.h"
#include "LanternParameters.h"

using Texture = ComPtr<Direct3DTexture8>;

namespace d3d
{
	extern VertexShader vertex_shader;
	extern PixelShader pixel_shader;

	extern bool do_effect;
	bool supports_xrgb();
	void reset_overrides();
	void load_shader();
	void set_flags(Uint32 flags, bool add = true);
	bool shaders_null();
	void init_trampolines();
}

namespace param
{
	extern Texture PaletteA;
	extern Texture PaletteB;

	extern PaletteParameters palette;
	extern LanternParameters lantern;
}

// Same as in the mod loader except with d3d8to9 types.
#pragma pack(push, 1)
struct MeshSetBuffer
{
	NJS_MESHSET_SADX *Meshset;
	void* field_4;
	int FVF;
	Direct3DVertexBuffer8* VertexBuffer;
	int Size;
	Direct3DIndexBuffer8* IndexBuffer;
	D3DPRIMITIVETYPE PrimitiveType;
	int MinIndex;
	int NumVertecies;
	int StartIndex;
	int PrimitiveCount;
};

struct __declspec(align(2)) PolyBuff_RenderArgs
{
	Uint32 StartVertex;
	Uint32 PrimitiveCount;
	Uint32 CullMode;
	Uint32 d;
};

struct PolyBuff
{
	Direct3DVertexBuffer8 *pStreamData;
	Uint32 TotalSize;
	Uint32 CurrentSize;
	Uint32 Stride;
	Uint32 FVF;
	PolyBuff_RenderArgs *RenderArgs;
	Uint32 LockCount;
	const char *name;
	int i;
};
#pragma pack(pop)
