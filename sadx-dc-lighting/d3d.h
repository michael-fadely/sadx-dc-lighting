#pragma once

#include <d3d9.h>
#include <d3dx9effect.h>
#include <d3d8to9.hpp>
#include <ninja.h>

namespace d3d
{
	extern IDirect3DDevice9* device;
	extern ID3DXEffect* effect;
	extern bool do_effect;
	void LoadShader();
	void InitTrampolines();
	void UpdateParameterHandles();
}

namespace param
{
	extern D3DXHANDLE BaseTexture;
	extern D3DXHANDLE DiffusePalette;
	extern D3DXHANDLE SpecularPalette;
	extern D3DXHANDLE WorldMatrix;
	extern D3DXHANDLE wvMatrix;
	extern D3DXHANDLE ProjectionMatrix;
	extern D3DXHANDLE wvMatrixInvT;
	extern D3DXHANDLE TextureTransform;
	extern D3DXHANDLE TextureEnabled;
	extern D3DXHANDLE EnvironmentMapped;
	extern D3DXHANDLE AlphaEnabled;
	extern D3DXHANDLE FogMode;
	extern D3DXHANDLE FogStart;
	extern D3DXHANDLE FogEnd;
	extern D3DXHANDLE FogDensity;
	extern D3DXHANDLE FogColor;
	extern D3DXHANDLE LightDirection;
	extern D3DXHANDLE LightLength;
	extern D3DXHANDLE DiffuseSource;
	extern D3DXHANDLE MaterialDiffuse;
	extern D3DXHANDLE AlphaRef;
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
