#pragma once

#include <d3d9.h>
#include <d3dx9effect.h>
#include <d3d8to9.hpp>
#include <ninja.h>

#include "lantern.h"
#include "ShaderParameter.h"

namespace d3d
{
	extern IDirect3DDevice9* device;
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
	extern ShaderParameter<Texture> PaletteA;
	extern ShaderParameter<Texture> PaletteB;

	extern ShaderParameter<D3DXVECTOR4> Indices;

	extern ShaderParameter<float> DiffuseBlendFactor;
	extern ShaderParameter<float> SpecularBlendFactor;
	extern ShaderParameter<D3DXMATRIX> WorldMatrix;
	extern ShaderParameter<D3DXMATRIX> ViewMatrix;
	extern ShaderParameter<D3DXMATRIX> ProjectionMatrix;
	extern ShaderParameter<D3DXMATRIX> wvMatrixInvT;
	extern ShaderParameter<D3DXMATRIX> TextureTransform;
	extern ShaderParameter<int> FogMode;
	extern ShaderParameter<float> FogStart;
	extern ShaderParameter<float> FogEnd;
	extern ShaderParameter<float> FogDensity;
	extern ShaderParameter<D3DXCOLOR> FogColor;
	extern ShaderParameter<D3DXVECTOR3> LightDirection;
	extern ShaderParameter<int> DiffuseSource;

	extern ShaderParameter<D3DXCOLOR> MaterialDiffuse;

	extern ShaderParameter<D3DXVECTOR3> NormalScale;
	extern ShaderParameter<bool> AllowVertexColor;
	extern ShaderParameter<bool> ForceDefaultDiffuse;
	extern ShaderParameter<bool> DiffuseOverride;
	extern ShaderParameter<D3DXVECTOR3> DiffuseOverrideColor;

#ifdef USE_SL
	extern ShaderParameter<D3DXCOLOR> MaterialSpecular;
	extern ShaderParameter<float> MaterialPower;
	extern ShaderParameter<SourceLight_t> SourceLight;
	extern ShaderParameter<StageLights> Lights;
#endif
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
