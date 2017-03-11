#pragma once

#include <d3d9.h>
#include <d3dx9effect.h>
#include <d3d8to9.hpp>
#include <ninja.h>

#include "lantern.h"
#include "EffectParameter.h"

#define USE_OIT

namespace d3d
{
	enum ShaderOptions : Uint32
	{
		None,
		UseTexture = 1 << 0,
		UseEnvMap  = 1 << 1,
		UseAlpha   = 1 << 2,
		UseLight   = 1 << 3,
		UseBlend   = 1 << 4,
		UseFog     = 1 << 5,
		UseOit     = 1 << 6,
		Mask       = 0x7F,
		Count
	};

	extern IDirect3DDevice9* device;
	extern Effect effect;
	extern bool do_effect;
	void LoadShader();
	void SetShaderOptions(Uint32 options, bool add = true);
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
	extern EffectParameter<D3DXMATRIX> wvMatrix;
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

#ifdef USE_OIT
	extern EffectParameter<Texture> AlphaDepth;
	extern EffectParameter<Texture> OpaqueDepth;
	extern EffectParameter<Texture> BackBuffer;
	extern EffectParameter<bool> AlphaDepthTest;
	extern EffectParameter<bool> DestBlendOne;
	extern EffectParameter<D3DXVECTOR2> ViewPort;
#endif

#ifdef USE_SL
	extern EffectParameter<D3DXCOLOR> MaterialSpecular;
	extern EffectParameter<float> MaterialPower;
	extern EffectParameter<SourceLight_t> SourceLight;
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
#pragma pack(pop)
