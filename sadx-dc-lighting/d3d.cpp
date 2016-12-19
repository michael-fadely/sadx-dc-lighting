#include "stdafx.h"

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"

#pragma pack(push, 1)
struct __declspec(align(2)) PolyBuff_RenderArgs
{
	Uint32 StartVertex;
	Uint32 PrimitiveCount;
	Uint32 CullMode;
	Uint32 d;
};
struct PolyBuff
{
	/*I*/Direct3DVertexBuffer8 *pStreamData;
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

enum TechniqueIndex : Uint32
{
	Standard = 0,
	NoLight  = 1 << 0,
	NoFog    = 1 << 1,
	Neither  = NoFog | NoLight
};

static UINT passes       = 0;
static bool initialized  = false;
static bool began_effect = false;
static Uint32 drawing    = 0;

static TechniqueIndex last_technique = Standard;
static D3DXHANDLE techniques[4] = {};

static Trampoline* CreateDirect3DDevice_t             = nullptr;
static Trampoline* sub_77EAD0_t                       = nullptr;
static Trampoline* sub_77EBA0_t                       = nullptr;
static Trampoline* njDrawModel_SADX_t                 = nullptr;
static Trampoline* njDrawModel_SADX_B_t               = nullptr;
static Trampoline* PolyBuff_DrawTriangleStrip_t       = nullptr;
static Trampoline* PolyBuff_DrawTriangleList_t        = nullptr;
static Trampoline* ProcessModelNode_t                 = nullptr;
static Trampoline* ProcessModelNode_B_t               = nullptr;
static Trampoline* ProcessModelNode_C_t               = nullptr;
static Trampoline* ProcessModelNode_D_t               = nullptr;
static Trampoline* Direct3D_PerformLighting_t         = nullptr;
static Trampoline* Direct3D_SetProjectionMatrix_t     = nullptr;
static Trampoline* Direct3D_SetTexList_t              = nullptr;
static Trampoline* Direct3D_SetViewportAndTransform_t = nullptr;
static Trampoline* Direct3D_SetWorldTransform_t       = nullptr;
static Trampoline* MeshSet_CreateVertexBuffer_t       = nullptr;

DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);
DataPointer(D3DXMATRIX, InverseViewMatrix, 0x0389D358);
DataPointer(D3DXMATRIX, TransformationMatrix, 0x03D0FD80);
DataPointer(D3DXMATRIX, ViewMatrix, 0x0389D398);
DataPointer(D3DXMATRIX, WorldMatrix, 0x03D12900);
DataPointer(D3DXMATRIX, _ProjectionMatrix, 0x03D129C0);
DataPointer(NJS_TEXLIST*, CommonTextures, 0x03B290B0);
DataArray(NJS_TEXLIST*, LevelObjTexlists, 0x03B290B4, 4);
DataPointer(int, TransformAndViewportInvalid, 0x03D0FD1C);

namespace d3d
{
	IDirect3DDevice9* device = nullptr;
	ID3DXEffect* effect      = nullptr;
	bool do_effect           = false;
}

namespace param
{
	D3DXHANDLE BaseTexture       = nullptr;
	D3DXHANDLE DiffusePalette    = nullptr;
	D3DXHANDLE DiffusePaletteB   = nullptr;
	D3DXHANDLE SpecularPalette   = nullptr;
	D3DXHANDLE SpecularPaletteB  = nullptr;
	D3DXHANDLE BlendFactor       = nullptr;
	D3DXHANDLE WorldMatrix       = nullptr;
	D3DXHANDLE wvMatrix          = nullptr;
	D3DXHANDLE ProjectionMatrix  = nullptr;
	D3DXHANDLE wvMatrixInvT      = nullptr;
	D3DXHANDLE TextureTransform  = nullptr;
	D3DXHANDLE TextureEnabled    = nullptr;
	D3DXHANDLE EnvironmentMapped = nullptr;
	D3DXHANDLE AlphaEnabled      = nullptr;
	D3DXHANDLE FogMode           = nullptr;
	D3DXHANDLE FogStart          = nullptr;
	D3DXHANDLE FogEnd            = nullptr;
	D3DXHANDLE FogDensity        = nullptr;
	D3DXHANDLE FogColor          = nullptr;
	D3DXHANDLE LightDirection    = nullptr;
	D3DXHANDLE LightLength       = nullptr;
	D3DXHANDLE DiffuseSource     = nullptr;
	D3DXHANDLE MaterialDiffuse   = nullptr;
	D3DXHANDLE AlphaRef          = nullptr;
}

using namespace d3d;

static void begin()
{
	if (!do_effect || began_effect || effect == nullptr)
		return;

	TechniqueIndex technique = (TechniqueIndex)((char)!globals::light | (char)!globals::fog << 1 & 2);
	if (technique != last_technique)
	{
		effect->SetTechnique(techniques[technique]);
		last_technique = technique;
	}

	effect->Begin(&passes, 0);

	if (passes > 1)
	{
		throw std::runtime_error("Multi-pass shaders are not supported.");
	}

	effect->BeginPass(0);
	began_effect = true;
}
static void end()
{
	if (!began_effect || effect == nullptr)
		return;

	effect->EndPass();
	effect->End();
	began_effect = false;
}

template<typename T, typename... Args>
void RunTrampoline(const T& original, Args... args)
{
	++drawing;

	original(args...);

	if (drawing > 0 && --drawing == 0)
	{
		end();
		do_effect = false;
	}
}

static void DrawPolyBuff(PolyBuff* _this, D3DPRIMITIVETYPE type)
{
	/*
	 * This isn't ideal where mod compatibility is concerned.
	 * Since we're not calling the trampoline, this must be the
	 * last mod loaded in order for things to work nicely.
	 */

	Uint32 cullmode = D3DCULL_FORCE_DWORD;

	auto args = _this->RenderArgs;

	for (auto i = _this->LockCount; i; --i)
	{
		if (args->CullMode != cullmode)
		{
			device->SetRenderState(D3DRS_CULLMODE, args->CullMode);
			cullmode = args->CullMode;

			if (i == _this->LockCount)
			{
				begin();
			}
			else
			{
				effect->CommitChanges();
			}
		}

		device->DrawPrimitive(type, args->StartVertex, args->PrimitiveCount);
		++args;
	}

	_this->LockCount = 0;
	end();
}

static void SetLightParameters()
{
	using namespace d3d;

	if (!UsePalette() || effect == nullptr)
		return;

	D3DLIGHT9 light;
	device->GetLight(0, &light);
	auto dir = -*(D3DXVECTOR3*)&light.Direction;
	auto mag = D3DXVec3Length(&dir);
	effect->SetValue(param::LightDirection, &dir, sizeof(D3DVECTOR));
	effect->SetFloat(param::LightLength, mag);
}

#pragma region Trampolines

static void __cdecl sub_77EAD0_r(void* a1, int a2, int a3)
{
	begin();
	RunTrampoline(TARGET_DYNAMIC(sub_77EAD0), a1, a2, a3);
	end();
}

static void __cdecl sub_77EBA0_r(void* a1, int a2, int a3)
{
	begin();
	RunTrampoline(TARGET_DYNAMIC(sub_77EBA0), a1, a2, a3);
	end();
}

static void __cdecl njDrawModel_SADX_r(NJS_MODEL_SADX* a1)
{
	RunTrampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);
}

static void __cdecl njDrawModel_SADX_B_r(NJS_MODEL_SADX* a1)
{
	RunTrampoline(TARGET_DYNAMIC(njDrawModel_SADX_B), a1);
}

static void __fastcall PolyBuff_DrawTriangleStrip_r(PolyBuff* _this)
{
	DrawPolyBuff(_this, D3DPT_TRIANGLESTRIP);
}

static void __fastcall PolyBuff_DrawTriangleList_r(PolyBuff* _this)
{
	DrawPolyBuff(_this, D3DPT_TRIANGLELIST);
}

static void __fastcall CreateDirect3DDevice_r(int a1, int behavior, int type)
{
	TARGET_DYNAMIC(CreateDirect3DDevice)(a1, behavior, type);
	if (Direct3D_Device != nullptr && !initialized)
	{
		device = Direct3D_Device->GetProxyInterface();
		initialized = true;
		LoadShader();
	}
}

static void __cdecl Direct3D_PerformLighting_r(int type)
{
	TARGET_DYNAMIC(Direct3D_PerformLighting)(0);

	if (effect == nullptr)
		return;

	globals::light = true;

	if (type != globals::light_type)
	{
		SetLightParameters();
	}

	SetPaletteLights(type, globals::no_specular ? NJD_FLAG_IGNORE_SPECULAR : 0);
}

static void __cdecl Direct3D_SetWorldTransform_r()
{
	TARGET_DYNAMIC(Direct3D_SetWorldTransform)();

	if (!UsePalette() || effect == nullptr)
		return;

	effect->SetMatrix(param::WorldMatrix, &WorldMatrix);

	auto wvMatrix = WorldMatrix * ViewMatrix;
	effect->SetMatrix(param::wvMatrix, &wvMatrix);

	D3DXMatrixInverse(&wvMatrix, nullptr, &wvMatrix);
	D3DXMatrixTranspose(&wvMatrix, &wvMatrix);
	// The inverse transpose matrix is used for environment mapping.
	effect->SetMatrix(param::wvMatrixInvT, &wvMatrix);
}

static Sint32 __fastcall Direct3D_SetTexList_r(NJS_TEXLIST* texlist)
{
	if (texlist != Direct3D_CurrentTexList)
	{
		globals::no_specular = false;

		if (!globals::light_type)
		{
			if (texlist == CommonTextures)
			{
				SetPaletteLights(0, 0);
			}
			else
			{
				for (int i = 0; i < 4; i++)
				{
					if (LevelObjTexlists[i] != texlist)
						continue;

					SetPaletteLights(0, NJD_FLAG_IGNORE_SPECULAR);
					globals::no_specular = true;
					break;
				}
			}
		}
	}

	return TARGET_DYNAMIC(Direct3D_SetTexList)(texlist);
}

static void __stdcall Direct3D_SetProjectionMatrix_r(float hfov, float nearPlane, float farPlane)
{
	TARGET_DYNAMIC(Direct3D_SetProjectionMatrix)(hfov, nearPlane, farPlane);

	if (effect == nullptr)
		return;

	// The view matrix can also be set here if necessary.
	auto m = _ProjectionMatrix * TransformationMatrix;
	effect->SetMatrix(param::ProjectionMatrix, &m);
}

static void __cdecl Direct3D_SetViewportAndTransform_r()
{
	auto original = TARGET_DYNAMIC(Direct3D_SetViewportAndTransform);
	bool invalid = TransformAndViewportInvalid != 0;
	original();

	if (effect != nullptr && invalid)
	{
		auto m = _ProjectionMatrix * TransformationMatrix;
		effect->SetMatrix(param::ProjectionMatrix, &m);
	}
}

#pragma endregion

static void __stdcall DrawMeshSetBuffer_c(MeshSetBuffer* buffer)
{
	if (!buffer->FVF)
		return;

	device->SetFVF(buffer->FVF);
	device->SetStreamSource(0, buffer->VertexBuffer->GetProxyInterface(), 0, buffer->Size);

	auto indexBuffer = buffer->IndexBuffer;
	if (indexBuffer)
	{
		device->SetIndices(indexBuffer->GetProxyInterface());

		begin();

		device->DrawIndexedPrimitive(
			buffer->PrimitiveType, 0,
			buffer->MinIndex,
			buffer->NumVertecies,
			buffer->StartIndex,
			buffer->PrimitiveCount);
	}
	else
	{
		begin();
		device->DrawPrimitive(
			buffer->PrimitiveType,
			buffer->StartIndex,
			buffer->PrimitiveCount);
	}

	end();
}

constexpr auto loc_77EF09 = (void*)0x0077EF09;
static void __declspec(naked) DrawMeshSetBuffer_asm()
{
	__asm
	{
		push esi
		call DrawMeshSetBuffer_c
		jmp  loc_77EF09
	}
}

static void MeshSet_CreateVertexBuffer_original(MeshSetBuffer* mesh, int count)
{
	// ReSharper disable once CppEntityNeverUsed
	auto original = MeshSet_CreateVertexBuffer_t->Target();
	__asm
	{
		mov  edi, mesh
		push count
		call original
		add  esp, 4
	}
}

// Overrides landtable vertex colors with white (retaining alpha channel)
static void __cdecl MeshSet_CreateVertexBuffer_c(MeshSetBuffer* mesh, int count)
{
	if (mesh->VertexBuffer == nullptr && mesh->Meshset->vertcolor != nullptr)
	{
		auto n = count;

		switch (mesh->Meshset->type_matId & NJD_MESHSET_MASK)
		{
			default:
				n = n - 2 * mesh->Meshset->nbMesh;
				break;

			case NJD_MESHSET_3:
				n = mesh->Meshset->nbMesh * 3;
				break;

			case NJD_MESHSET_4:
				n = mesh->Meshset->nbMesh * 4;
				break;
		}

		for (int i = 0; i < n; i++)
		{
			auto& color = mesh->Meshset->vertcolor[i].color;

			if ((color & 0x00FFFFFF) == 0x00B2B2B2)
			{
				color |= 0x00FFFFFF;
			}
		}
	}

	MeshSet_CreateVertexBuffer_original(mesh, count);
}
static void __declspec(naked) MeshSet_CreateVertexBuffer_r()
{
	__asm
	{
		push [esp + 04h]  // count
		push edi          // mesh
		call MeshSet_CreateVertexBuffer_c
		pop  edi          // mesh
		add  esp, 4       // count
		retn
	}
}

static auto __stdcall SetTransformHijack(Direct3DDevice8* _device, D3DTRANSFORMSTATETYPE type, D3DXMATRIX* matrix)
{
	if (effect != nullptr)
	{
		effect->SetMatrix(param::ProjectionMatrix, matrix);
	}

	return device->SetTransform(type, matrix);
}

void d3d::UpdateParameterHandles()
{
#define DOTHINGPLS(name) \
	::param::##name = effect->GetParameterByName(nullptr, #name);

	// Texture stuff:

	DOTHINGPLS(BaseTexture);

	DOTHINGPLS(DiffusePalette);
	DOTHINGPLS(DiffusePaletteB);

	DOTHINGPLS(SpecularPalette);
	DOTHINGPLS(SpecularPaletteB);

	DOTHINGPLS(BlendFactor);

	// Other things:

	DOTHINGPLS(WorldMatrix);
	DOTHINGPLS(wvMatrix);
	DOTHINGPLS(ProjectionMatrix);
	DOTHINGPLS(wvMatrixInvT);
	DOTHINGPLS(TextureTransform);
	DOTHINGPLS(TextureEnabled);
	DOTHINGPLS(EnvironmentMapped);
	DOTHINGPLS(AlphaEnabled);
	DOTHINGPLS(FogMode);
	DOTHINGPLS(FogStart);
	DOTHINGPLS(FogEnd);
	DOTHINGPLS(FogDensity);
	DOTHINGPLS(FogColor);
	DOTHINGPLS(LightDirection);
	DOTHINGPLS(LightLength);
	DOTHINGPLS(DiffuseSource);
	DOTHINGPLS(MaterialDiffuse);
	DOTHINGPLS(AlphaRef);
}

void d3d::LoadShader()
{
	if (!initialized)
		return;

	ID3DXBuffer* pCompilationErrors = nullptr;

	try
	{
		if (effect != nullptr)
		{
			effect->Release();
			effect = nullptr;
		}

		auto path = globals::system + "lantern.fx";

		auto result = D3DXCreateEffectFromFileA(device, path.c_str(), nullptr, nullptr,
			D3DXFX_NOT_CLONEABLE, nullptr, &effect, &pCompilationErrors);

		if (FAILED(result))
		{
			if (pCompilationErrors)
			{
				std::string compilationErrors(static_cast<const char*>(
					pCompilationErrors->GetBufferPointer()));

				pCompilationErrors->Release();
				throw std::runtime_error(compilationErrors);
			}
		}

		if (pCompilationErrors)
		{
			pCompilationErrors->Release();
		}

		if (effect == nullptr)
		{
			throw std::runtime_error("Shader creation failed with an unknown error. (Does " + path + " exist?)");
		}

		UpdateParameterHandles();

		techniques[0] = effect->GetTechniqueByName("Standard");
		techniques[1] = effect->GetTechniqueByName("NoLight");
		techniques[2] = effect->GetTechniqueByName("NoFog");
		techniques[3] = effect->GetTechniqueByName("Neither");

		effect->SetTechnique(techniques[0]);
	}
	catch (std::exception& ex)
	{
		effect = nullptr;
		MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
	}
}

void d3d::InitTrampolines()
{
	CreateDirect3DDevice_t             = new Trampoline(0x00794000, 0x00794007, CreateDirect3DDevice_r);
	sub_77EAD0_t                       = new Trampoline(0x0077EAD0, 0x0077EAD7, sub_77EAD0_r);
	sub_77EBA0_t                       = new Trampoline(0x0077EBA0, 0x0077EBA5, sub_77EBA0_r);
	njDrawModel_SADX_t                 = new Trampoline(0x0077EDA0, 0x0077EDAA, njDrawModel_SADX_r);
	njDrawModel_SADX_B_t               = new Trampoline(0x00784AE0, 0x00784AE5, njDrawModel_SADX_B_r);
	PolyBuff_DrawTriangleStrip_t       = new Trampoline(0x00794760, 0x00794767, PolyBuff_DrawTriangleStrip_r);
	PolyBuff_DrawTriangleList_t        = new Trampoline(0x007947B0, 0x007947B7, PolyBuff_DrawTriangleList_r);
	Direct3D_PerformLighting_t         = new Trampoline(0x00412420, 0x00412426, Direct3D_PerformLighting_r);
	Direct3D_SetProjectionMatrix_t     = new Trampoline(0x00791170, 0x00791175, Direct3D_SetProjectionMatrix_r);
	Direct3D_SetTexList_t              = new Trampoline(0x0077F3D0, 0x0077F3D8, Direct3D_SetTexList_r);
	Direct3D_SetViewportAndTransform_t = new Trampoline(0x007912E0, 0x007912E8, Direct3D_SetViewportAndTransform_r);
	Direct3D_SetWorldTransform_t       = new Trampoline(0x00791AB0, 0x00791AB5, Direct3D_SetWorldTransform_r);
	MeshSet_CreateVertexBuffer_t       = new Trampoline(0x007853D0, 0x007853D6, MeshSet_CreateVertexBuffer_r);

	WriteJump((void*)0x0077EE45, DrawMeshSetBuffer_asm);

	// Hijacking a IDirect3DDevice8::SetTransform call in Direct3D_SetNearFarPlanes
	// to update the projection matrix.
	// This nops:
	// mov ecx, [eax] (device)
	// call dword ptr [ecx+94h] (device->SetTransform)
	WriteData((void*)0x00403234, 0x90i8, 8);
	WriteCall((void*)0x00403236, SetTransformHijack);
}
