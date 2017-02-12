#include "stdafx.h"

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <Trampoline.h>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"
#include "EffectParameter.h"

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

static Trampoline* Direct3D_PerformLighting_t         = nullptr;
static Trampoline* sub_77EAD0_t                       = nullptr;
static Trampoline* sub_77EBA0_t                       = nullptr;
static Trampoline* njDrawModel_SADX_t                 = nullptr;
static Trampoline* njDrawModel_SADX_B_t               = nullptr;
static Trampoline* Direct3D_SetProjectionMatrix_t     = nullptr;
static Trampoline* Direct3D_SetViewportAndTransform_t = nullptr;
static Trampoline* Direct3D_SetWorldTransform_t       = nullptr;
static Trampoline* CreateDirect3DDevice_t             = nullptr;
static Trampoline* PolyBuff_DrawTriangleStrip_t       = nullptr;
static Trampoline* PolyBuff_DrawTriangleList_t        = nullptr;

DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);
DataPointer(D3DXMATRIX, InverseViewMatrix, 0x0389D358);
DataPointer(D3DXMATRIX, TransformationMatrix, 0x03D0FD80);
DataPointer(D3DXMATRIX, ViewMatrix, 0x0389D398);
DataPointer(D3DXMATRIX, WorldMatrix, 0x03D12900);
DataPointer(D3DXMATRIX, _ProjectionMatrix, 0x03D129C0);
DataPointer(int, TransformAndViewportInvalid, 0x03D0FD1C);

namespace d3d
{
	IDirect3DDevice9* device = nullptr;
	ID3DXEffect* effect      = nullptr;
	bool do_effect           = false;
}

namespace param
{
	EffectParameter<Texture> BaseTexture(&d3d::effect, "BaseTexture", nullptr);

	EffectParameter<Texture> PaletteA(&d3d::effect, "PaletteA", nullptr);
	EffectParameter<float> DiffuseIndexA(&d3d::effect, "DiffuseIndexA", 0.0f);
	EffectParameter<float> SpecularIndexA(&d3d::effect, "SpecularIndexA", 0.0f);

	EffectParameter<Texture> PaletteB(&d3d::effect, "PaletteB", nullptr);
	EffectParameter<float> DiffuseIndexB(&d3d::effect, "DiffuseIndexB", 0.0f);
	EffectParameter<float> SpecularIndexB(&d3d::effect, "SpecularIndexB", 0.0f);

	EffectParameter<float> BlendFactor(&d3d::effect, "BlendFactor", 0.0f);
	EffectParameter<D3DXMATRIX> WorldMatrix(&d3d::effect, "WorldMatrix", {});
	EffectParameter<D3DXMATRIX> wvMatrix(&d3d::effect, "wvMatrix", {});
	EffectParameter<D3DXMATRIX> ProjectionMatrix(&d3d::effect, "ProjectionMatrix", {});
	EffectParameter<D3DXMATRIX> wvMatrixInvT(&d3d::effect, "wvMatrixInvT", {});
	EffectParameter<D3DXMATRIX> TextureTransform(&d3d::effect, "TextureTransform", {});
	EffectParameter<bool> TextureEnabled(&d3d::effect, "TextureEnabled", true);
	EffectParameter<bool> EnvironmentMapped(&d3d::effect, "EnvironmentMapped", false);
	EffectParameter<bool> AlphaEnabled(&d3d::effect, "AlphaEnabled", true);
	EffectParameter<int> FogMode(&d3d::effect, "FogMode", 0);
	EffectParameter<float> FogStart(&d3d::effect, "FogStart", 0.0f);
	EffectParameter<float> FogEnd(&d3d::effect, "FogEnd", 0.0f);
	EffectParameter<float> FogDensity(&d3d::effect, "FogDensity", 0.0f);
	EffectParameter<D3DXCOLOR> FogColor(&d3d::effect, "FogColor", {});
	EffectParameter<D3DXVECTOR3> LightDirection(&d3d::effect, "LightDirection", {});
	EffectParameter<int> DiffuseSource(&d3d::effect, "DiffuseSource", 0);

	EffectParameter<D3DXCOLOR> MaterialDiffuse(&d3d::effect, "MaterialDiffuse", {});

#ifdef USE_SL
	EffectParameter<D3DXCOLOR> MaterialSpecular(&d3d::effect, "MaterialSpecular", {});
	EffectParameter<float> MaterialPower(&d3d::effect, "MaterialPower", 1.0f);
	EffectParameter<bool> UseSourceLight(&d3d::effect, "UseSourceLight", false);
	EffectParameter<SourceLight_t> SourceLight(&d3d::effect, "SourceLight", {});
#endif

	EffectParameter<float> AlphaRef(&d3d::effect, "AlphaRef", 0.0f);
	EffectParameter<D3DXVECTOR3> NormalScale(&d3d::effect, "NormalScale", { 1.0f, 1.0f, 1.0f });

	static IEffectParameter* const parameters[] = {
		&BaseTexture,

		&PaletteA,
		&DiffuseIndexA,
		&SpecularIndexA,

		&PaletteB,
		&DiffuseIndexB,
		&SpecularIndexB,
		
		&BlendFactor,
		&WorldMatrix,
		&wvMatrix,
		&ProjectionMatrix,
		&wvMatrixInvT,
		&TextureTransform,
		&TextureEnabled,
		&EnvironmentMapped,
		&AlphaEnabled,
		&FogMode,
		&FogStart,
		&FogEnd,
		&FogDensity,
		&FogColor,
		&LightDirection,
		&DiffuseSource,
		&MaterialDiffuse,
		&AlphaRef,
		&NormalScale,

#ifdef USE_SL
		&SourceLight,
		&MaterialSpecular,
		&MaterialPower,
		&UseSourceLight,
#endif
	};
}

using namespace d3d;

static void begin()
{
	if (!do_effect || began_effect || effect == nullptr)
	{
		return;
	}

	TechniqueIndex technique = (TechniqueIndex)((char)!globals::light | (char)!globals::fog << 1 & 2);
	if (technique != last_technique)
	{
		effect->SetTechnique(techniques[technique]);
		last_technique = technique;
	}

	for (auto i : param::parameters)
	{
		i->Commit();
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
	{
		return;
	}

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
	if (!LanternInstance::UsePalette() || effect == nullptr)
	{
		return;
	}

	D3DLIGHT9 light;
	device->GetLight(0, &light);
	param::LightDirection = -*(D3DXVECTOR3*)&light.Direction;
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

static void __cdecl Direct3D_SetWorldTransform_r()
{
	TARGET_DYNAMIC(Direct3D_SetWorldTransform)();

	if (!LanternInstance::UsePalette() || effect == nullptr)
	{
		return;
	}

	param::WorldMatrix = WorldMatrix;

	auto wvMatrix = WorldMatrix * ViewMatrix;
	param::wvMatrix = wvMatrix;

	D3DXMatrixInverse(&wvMatrix, nullptr, &wvMatrix);
	D3DXMatrixTranspose(&wvMatrix, &wvMatrix);
	// The inverse transpose matrix is used for environment mapping.
	param::wvMatrixInvT = wvMatrix;
}

static void __stdcall Direct3D_SetProjectionMatrix_r(float hfov, float nearPlane, float farPlane)
{
	TARGET_DYNAMIC(Direct3D_SetProjectionMatrix)(hfov, nearPlane, farPlane);

	if (effect == nullptr)
	{
		return;
	}

	// The view matrix can also be set here if necessary.
	param::ProjectionMatrix = _ProjectionMatrix * TransformationMatrix;
}

static void __cdecl Direct3D_SetViewportAndTransform_r()
{
	auto original = TARGET_DYNAMIC(Direct3D_SetViewportAndTransform);
	bool invalid = TransformAndViewportInvalid != 0;
	original();

	if (effect != nullptr && invalid)
	{
		param::ProjectionMatrix = _ProjectionMatrix * TransformationMatrix;
	}
}

static void __cdecl Direct3D_PerformLighting_r(int type)
{
	auto target = TARGET_DYNAMIC(Direct3D_PerformLighting);

	if (effect == nullptr || !LanternInstance::UsePalette())
	{
		target(type);
		return;
	}

	// This specifically force light type 0 to prevent
	// the light direction from being overwritten.
	target(0);
	globals::light = true;

	if (type != globals::light_type)
	{
		SetLightParameters();
	}

	globals::palettes.SetPalettes(type, globals::no_specular ? NJD_FLAG_IGNORE_SPECULAR : 0);
}

#pragma endregion

static void __stdcall DrawMeshSetBuffer_c(MeshSetBuffer* buffer)
{
	if (!buffer->FVF)
	{
		return;
	}

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

static auto __stdcall SetTransformHijack(Direct3DDevice8* _device, D3DTRANSFORMSTATETYPE type, D3DXMATRIX* matrix)
{
	if (effect != nullptr)
	{
		param::ProjectionMatrix = *matrix;
	}

	return device->SetTransform(type, matrix);
}

static void UpdateParameterHandles()
{
	for (auto i : param::parameters)
	{
		i->UpdateHandle();
	}
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
	Direct3D_PerformLighting_t         = new Trampoline(0x00412420, 0x00412426, Direct3D_PerformLighting_r);
	sub_77EAD0_t                       = new Trampoline(0x0077EAD0, 0x0077EAD7, sub_77EAD0_r);
	sub_77EBA0_t                       = new Trampoline(0x0077EBA0, 0x0077EBA5, sub_77EBA0_r);
	njDrawModel_SADX_t                 = new Trampoline(0x0077EDA0, 0x0077EDAA, njDrawModel_SADX_r);
	njDrawModel_SADX_B_t               = new Trampoline(0x00784AE0, 0x00784AE5, njDrawModel_SADX_B_r);
	Direct3D_SetProjectionMatrix_t     = new Trampoline(0x00791170, 0x00791175, Direct3D_SetProjectionMatrix_r);
	Direct3D_SetViewportAndTransform_t = new Trampoline(0x007912E0, 0x007912E8, Direct3D_SetViewportAndTransform_r);
	Direct3D_SetWorldTransform_t       = new Trampoline(0x00791AB0, 0x00791AB5, Direct3D_SetWorldTransform_r);
	CreateDirect3DDevice_t             = new Trampoline(0x00794000, 0x00794007, CreateDirect3DDevice_r);
	PolyBuff_DrawTriangleStrip_t       = new Trampoline(0x00794760, 0x00794767, PolyBuff_DrawTriangleStrip_r);
	PolyBuff_DrawTriangleList_t        = new Trampoline(0x007947B0, 0x007947B7, PolyBuff_DrawTriangleList_r);

	WriteJump((void*)0x0077EE45, DrawMeshSetBuffer_asm);

	// Hijacking a IDirect3DDevice8::SetTransform call in Direct3D_SetNearFarPlanes
	// to update the projection matrix.
	// This nops:
	// mov ecx, [eax] (device)
	// call dword ptr [ecx+94h] (device->SetTransform)
	WriteData((void*)0x00403234, 0x90i8, 8);
	WriteCall((void*)0x00403236, SetTransformHijack);
}

// These exports are for the window resize branch of the mod loader.
extern "C"
{
	EXPORT void __cdecl OnRenderDeviceLost()
	{
		if (effect != nullptr)
		{
			effect->OnLostDevice();
		}
	}

	EXPORT void __cdecl OnRenderDeviceReset()
	{
		if (effect != nullptr)
		{
			effect->OnResetDevice();
			UpdateParameterHandles();
		}
	}
}
