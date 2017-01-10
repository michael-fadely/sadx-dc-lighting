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
	extern ID3DXEffect* effect;
	extern bool do_effect;
	void LoadShader();
	void InitTrampolines();
}

namespace param
{
	extern EffectParameter<IDirect3DTexture9*> BaseTexture;
	extern EffectParameter<IDirect3DTexture9*> DiffusePalette;
	extern EffectParameter<IDirect3DTexture9*> DiffusePaletteB;
	extern EffectParameter<IDirect3DTexture9*> SpecularPalette;
	extern EffectParameter<IDirect3DTexture9*> SpecularPaletteB;
	extern EffectParameter<float> BlendFactor;
	extern EffectParameter<D3DXMATRIX> WorldMatrix;
	extern EffectParameter<D3DXMATRIX> wvMatrix;
	extern EffectParameter<D3DXMATRIX> ProjectionMatrix;
	extern EffectParameter<D3DXMATRIX> wvMatrixInvT;
	extern EffectParameter<D3DXMATRIX> TextureTransform;
	extern EffectParameter<bool> TextureEnabled;
	extern EffectParameter<bool> EnvironmentMapped;
	extern EffectParameter<bool> AlphaEnabled;
	extern EffectParameter<int> FogMode;
	extern EffectParameter<float> FogStart;
	extern EffectParameter<float> FogEnd;
	extern EffectParameter<float> FogDensity;
	extern EffectParameter<D3DXCOLOR> FogColor;
	extern EffectParameter<D3DXVECTOR3> LightDirection;
	extern EffectParameter<float> LightLength;
	extern EffectParameter<int> DiffuseSource;
	extern EffectParameter<D3DXCOLOR> MaterialDiffuse;
	extern EffectParameter<float> AlphaRef;
	extern EffectParameter<D3DXVECTOR3> NormalScale;
	extern EffectParameter<bool> UseSourceLight;
	extern EffectParameter<SourceLight_t> SourceLight;
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
#pragma pack(pop)
