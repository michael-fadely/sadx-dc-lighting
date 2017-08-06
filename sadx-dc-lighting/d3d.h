#pragma once

#include <d3d9.h>
#include <d3dx9effect.h>
#include <d3d8to9.hpp>
#include <ninja.h>

#include "lantern.h"
#include "EffectParameter.h"

namespace d3d
{
	extern IDirect3DDevice9* device;
	extern Effect effect;
	extern bool do_effect;
	void LoadShader();
	void SetShaderFlags(Uint32 flags, bool add = true);
	void InitTrampolines();
}

namespace param
{
	extern EffectParameter<Texture> BaseTexture;

	extern EffectParameter<Texture> PaletteA;
	extern EffectParameter<float> DiffuseIndexA;
	extern EffectParameter<float> SpecularIndexA;


	extern EffectParameter<Texture> PaletteB;
	extern EffectParameter<float> DiffuseIndexB;
	extern EffectParameter<float> SpecularIndexB;

	extern EffectParameter<float> BlendFactor;
	extern EffectParameter<D3DXMATRIX> WorldMatrix;
	extern EffectParameter<D3DXMATRIX> ViewMatrix;
	extern EffectParameter<D3DXMATRIX> ProjectionMatrix;
	extern EffectParameter<D3DXMATRIX> wvMatrixInvT;
	extern EffectParameter<D3DXMATRIX> TextureTransform;
	extern EffectParameter<int> FogMode;
	extern EffectParameter<float> FogStart;
	extern EffectParameter<float> FogEnd;
	extern EffectParameter<float> FogDensity;
	extern EffectParameter<D3DXCOLOR> FogColor;
	extern EffectParameter<D3DXVECTOR3> LightDirection;
	extern EffectParameter<int> DiffuseSource;

	extern EffectParameter<D3DXCOLOR> MaterialDiffuse;

	extern EffectParameter<float> AlphaRef;
	extern EffectParameter<D3DXVECTOR3> NormalScale;
	extern EffectParameter<bool> AllowVertexColor;
	extern EffectParameter<bool> ForceDefaultDiffuse;
	extern EffectParameter<bool> DiffuseOverride;
	extern EffectParameter<D3DXVECTOR3> DiffuseOverrideColor;

#ifdef USE_SL
	extern EffectParameter<D3DXCOLOR> MaterialSpecular;
	extern EffectParameter<float> MaterialPower;
	extern EffectParameter<SourceLight_t> SourceLight;
	extern EffectParameter<StageLights> Lights;
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
