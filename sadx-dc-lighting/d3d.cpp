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

#define ORIGINAL(name) ((decltype(name##_r)*)name##_t->Target())

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


IDirect3DDevice9* d3d::device = nullptr;
ID3DXEffect* d3d::effect      = nullptr;
bool d3d::do_effect           = false;

static UINT passes       = 0;
static bool initialized  = false;
static bool began_effect = false;
static Uint32 drawing    = 0;

static D3DXHANDLE techniques[4] = {};

static Trampoline* CreateDirect3DDevice_t       = nullptr;
static Trampoline* sub_77EAD0_t                 = nullptr;
static Trampoline* sub_77EBA0_t                 = nullptr;
static Trampoline* njDrawModel_SADX_t           = nullptr;
static Trampoline* njDrawModel_SADX_B_t         = nullptr;
static Trampoline* PolyBuff_DrawTriangleStrip_t = nullptr;
static Trampoline* PolyBuff_DrawTriangleList_t  = nullptr;
static Trampoline* ProcessModelNode_t           = nullptr;
static Trampoline* ProcessModelNode_B_t         = nullptr;
static Trampoline* ProcessModelNode_C_t         = nullptr;
static Trampoline* ProcessModelNode_D_t         = nullptr;

DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);

using namespace d3d;

enum TechniqueIndex : Uint32
{
	Standard = 0,
	NoLight  = 1 << 0,
	NoFog    = 1 << 1,
	Neither  = NoFog | NoLight
};

static void begin()
{
	if (!do_effect || began_effect || effect == nullptr)
		return;

	TechniqueIndex t = (TechniqueIndex)((char)!globals::light | (char)!globals::fog << 1 & 2);

	effect->SetTechnique(techniques[t]);
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
			effect->CommitChanges();
		}

		device->DrawPrimitive(type, args->StartVertex, args->PrimitiveCount);
		++args;
	}

	_this->LockCount = 0;
}

#pragma region Trampolines
static void __cdecl sub_77EAD0_r(void* a1, int a2, int a3)
{
	begin();
	RunTrampoline(ORIGINAL(sub_77EAD0), a1, a2, a3);
	end();
}

static void __cdecl sub_77EBA0_r(void* a1, int a2, int a3)
{
	begin();
	RunTrampoline(ORIGINAL(sub_77EBA0), a1, a2, a3);
	end();
}

static void __cdecl njDrawModel_SADX_r(NJS_MODEL_SADX* a1)
{
	RunTrampoline(ORIGINAL(njDrawModel_SADX), a1);
}

static void __cdecl njDrawModel_SADX_B_r(NJS_MODEL_SADX* a1)
{
	RunTrampoline(ORIGINAL(njDrawModel_SADX_B), a1);
}

static void __fastcall PolyBuff_DrawTriangleStrip_r(PolyBuff* _this)
{
	begin();
	DrawPolyBuff(_this, D3DPT_TRIANGLESTRIP);
	end();
}

static void __fastcall PolyBuff_DrawTriangleList_r(PolyBuff* _this)
{
	begin();
	DrawPolyBuff(_this, D3DPT_TRIANGLELIST);
	end();
}

#pragma endregion

static void __fastcall CreateDirect3DDevice_r(int a1, int behavior, int type)
{
	ORIGINAL(CreateDirect3DDevice)(a1, behavior, type);
	if (Direct3D_Device != nullptr && !initialized)
	{
		device = Direct3D_Device->GetProxyInterface();
		initialized = true;
		LoadShader();
	}
}

static void __cdecl DrawMeshSetBuffer_c(MeshSetBuffer* buffer)
{
	if (!buffer->FVF)
		return;

	Direct3D_Device->SetVertexShader(buffer->FVF);
	Direct3D_Device->SetStreamSource(0, buffer->VertexBuffer, buffer->Size);

	auto indexBuffer = buffer->IndexBuffer;
	if (indexBuffer)
	{
		Direct3D_Device->SetIndices(indexBuffer, 0);

		begin();

		Direct3D_Device->DrawIndexedPrimitive(
			buffer->PrimitiveType,
			buffer->MinIndex,
			buffer->NumVertecies,
			buffer->StartIndex,
			buffer->PrimitiveCount);
	}
	else
	{
		begin();
		Direct3D_Device->DrawPrimitive(
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
		pop esi
		jmp loc_77EF09
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

		techniques[0] = effect->GetTechniqueByName("Standard");
		techniques[1] = effect->GetTechniqueByName("NoLight");
		techniques[2] = effect->GetTechniqueByName("NoFog");
		techniques[3] = effect->GetTechniqueByName("Neither");
	}
	catch (std::exception& ex)
	{
		effect = nullptr;
//#ifdef _DEBUG
		//PrintDebug("Shader creation failed: %s\n", ex.what());
//#else
		MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
//#endif
	}
}

void d3d::InitTrampolines()
{
	CreateDirect3DDevice_t       = new Trampoline(0x00794000, 0x00794007, CreateDirect3DDevice_r);
	sub_77EAD0_t                 = new Trampoline(0x0077EAD0, 0x0077EAD7, sub_77EAD0_r);
	sub_77EBA0_t                 = new Trampoline(0x0077EBA0, 0x0077EBA5, sub_77EBA0_r);
	njDrawModel_SADX_t           = new Trampoline(0x0077EDA0, 0x0077EDAA, njDrawModel_SADX_r);
	njDrawModel_SADX_B_t         = new Trampoline(0x00784AE0, 0x00784AE5, njDrawModel_SADX_B_r);
	PolyBuff_DrawTriangleStrip_t = new Trampoline(0x00794760, 0x00794767, PolyBuff_DrawTriangleStrip_r);
	PolyBuff_DrawTriangleList_t  = new Trampoline(0x007947B0, 0x007947B7, PolyBuff_DrawTriangleList_r);

	WriteJump((void*)0x0077EE45, DrawMeshSetBuffer_asm);
}
