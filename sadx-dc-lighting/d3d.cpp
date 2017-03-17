#include "stdafx.h"

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXModLoader/SADXFunctions.h>
#include <Trampoline.h>

// MinHook
#include <MinHook.h>

// Standard library
#include <vector>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"
#include "EffectParameter.h"

namespace param
{
	EffectParameter<Texture>     BaseTexture("BaseTexture", nullptr);
	EffectParameter<Texture>     PaletteA("PaletteA", nullptr);
	EffectParameter<float>       DiffuseIndexA("DiffuseIndexA", 0.0f);
	EffectParameter<float>       SpecularIndexA("SpecularIndexA", 0.0f);
	EffectParameter<Texture>     PaletteB("PaletteB", nullptr);
	EffectParameter<float>       DiffuseIndexB("DiffuseIndexB", 0.0f);
	EffectParameter<float>       SpecularIndexB("SpecularIndexB", 0.0f);
	EffectParameter<float>       BlendFactor("BlendFactor", 0.0f);
	EffectParameter<D3DXMATRIX>  WorldMatrix("WorldMatrix", {});
	EffectParameter<D3DXMATRIX>  wvMatrix("wvMatrix", {});
	EffectParameter<D3DXMATRIX>  ProjectionMatrix("ProjectionMatrix", {});
	EffectParameter<D3DXMATRIX>  wvMatrixInvT("wvMatrixInvT", {});
	EffectParameter<D3DXMATRIX>  TextureTransform("TextureTransform", {});
	EffectParameter<int>         FogMode("FogMode", 0);
	EffectParameter<float>       FogStart("FogStart", 0.0f);
	EffectParameter<float>       FogEnd("FogEnd", 0.0f);
	EffectParameter<float>       FogDensity("FogDensity", 0.0f);
	EffectParameter<D3DXCOLOR>   FogColor("FogColor", {});
	EffectParameter<D3DXVECTOR3> LightDirection("LightDirection", {});
	EffectParameter<int>         DiffuseSource("DiffuseSource", 0);
	EffectParameter<D3DXCOLOR>   MaterialDiffuse("MaterialDiffuse", {});
	EffectParameter<float>       AlphaRef("AlphaRef", 0.0f);
	EffectParameter<D3DXVECTOR3> NormalScale("NormalScale", { 1.0f, 1.0f, 1.0f });

#ifdef USE_SL
	EffectParameter<D3DXCOLOR> MaterialSpecular("MaterialSpecular", {});
	EffectParameter<float> MaterialPower("MaterialPower", 1.0f);
	EffectParameter<SourceLight_t> SourceLight("SourceLight", {});
#endif

	IEffectParameter* const parameters[] = {
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

namespace local
{
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

	static HRESULT __stdcall DrawPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT StartVertex,
		UINT PrimitiveCount);
	static HRESULT __stdcall DrawIndexedPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		INT BaseVertexIndex,
		UINT MinVertexIndex,
		UINT NumVertices,
		UINT startIndex,
		UINT primCount);
	static HRESULT __stdcall DrawPrimitiveUP_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT PrimitiveCount,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride);
	static HRESULT __stdcall DrawIndexedPrimitiveUP_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT MinVertexIndex,
		UINT NumVertices,
		UINT PrimitiveCount,
		CONST void* pIndexData,
		D3DFORMAT IndexDataFormat,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride);

	static decltype(DrawPrimitive_r)*          DrawPrimitive_t          = nullptr;
	static decltype(DrawIndexedPrimitive_r)*   DrawIndexedPrimitive_t   = nullptr;
	static decltype(DrawPrimitiveUP_r)*        DrawPrimitiveUP_t        = nullptr;
	static decltype(DrawIndexedPrimitiveUP_r)* DrawIndexedPrimitiveUP_t = nullptr;

	constexpr auto DEFAULT_OPTIONS = d3d::UseAlpha | d3d::UseFog | d3d::UseLight | d3d::UseTexture;

	static Uint32 shader_options = DEFAULT_OPTIONS;
	static Uint32 last_options = DEFAULT_OPTIONS;

	static std::vector<Uint8> shaderFile;
	static Uint32 shaderCount = 0;
	static Effect shaders[d3d::ShaderOptions::Count] = {};
	static CComPtr<ID3DXEffectPool> pool = nullptr;

	static bool initialized = false;
	static Uint32 drawing = 0;
	static bool beganEffect = false;
	static std::vector<D3DXMACRO> macros;

	DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);
	DataPointer(D3DXMATRIX, InverseViewMatrix, 0x0389D358);
	DataPointer(D3DXMATRIX, TransformationMatrix, 0x03D0FD80);
	DataPointer(D3DXMATRIX, ViewMatrix, 0x0389D398);
	DataPointer(D3DXMATRIX, WorldMatrix, 0x03D12900);
	DataPointer(D3DXMATRIX, _ProjectionMatrix, 0x03D129C0);
	DataPointer(int, TransformAndViewportInvalid, 0x03D0FD1C);

	static auto sanitize(Uint32& options)
	{
		return options &= d3d::Mask;
	}

	static void updateHandles()
	{
		for (auto i : param::parameters)
		{
			i->UpdateHandle(d3d::effect);
		}
	}

	static void releaseParameters()
	{
		for (auto& i : param::parameters)
		{
			i->Release();
		}
	}

	static void releaseShaders()
	{
		shaderFile.clear();
		d3d::effect = nullptr;

		for (auto& e : shaders)
		{
			e = nullptr;
		}

		shaderCount = 0;
		pool = nullptr;
	}

	static Effect compileShader(Uint32 options)
	{
		sanitize(options);
		PrintDebug("[lantern] Compiling shader #%02d: %08X\n", ++shaderCount, options);

		if (pool == nullptr)
		{
			if (FAILED(D3DXCreateEffectPool(&pool)))
			{
				throw std::runtime_error("Failed to create effect pool?!");
			}
		}

		macros.clear();
		auto o = options;

		while (o != 0)
		{
			using namespace d3d;

			if (o & UseTexture)
			{
				o &= ~UseTexture;
				macros.push_back({ "USE_TEXTURE", "1" });
				continue;
			}

			if (o & UseEnvMap)
			{
				o &= ~UseEnvMap;
				macros.push_back({ "USE_ENVMAP", "1" });
				continue;
			}

			if (o & UseLight)
			{
				o &= ~UseLight;
				macros.push_back({ "USE_LIGHT", "1" });
				continue;
			}

			if (o & UseBlend)
			{
				o &= ~UseBlend;
				macros.push_back({ "USE_BLEND", "1" });
				continue;
			}

			if (o & UseAlpha)
			{
				o &= ~UseAlpha;
				macros.push_back({ "USE_ALPHA", "1" });
				continue;
			}

			if (o & UseFog)
			{
				o &= ~UseFog;
				macros.push_back({ "USE_FOG", "1" });
				continue;
			}

			break;
		}

		macros.push_back({});

		ID3DXBuffer* errors = nullptr;
		Effect effect;

		auto path = globals::system + "lantern.fx";

		if (shaderFile.empty())
		{
			std::ifstream file(path, std::ios::ate);
			auto size = file.tellg();
			file.seekg(0);

			if (file.is_open() && size > 0)
			{
				shaderFile.resize((size_t)size);
				file.read((char*)shaderFile.data(), size);
			}

			file.close();
		}

		if (!shaderFile.empty())
		{
			auto result = D3DXCreateEffect(d3d::device, shaderFile.data(), shaderFile.size(), macros.data(), nullptr,
				D3DXFX_NOT_CLONEABLE | D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE,
				pool, &effect, &errors);

			if (FAILED(result) || errors)
			{
				if (errors)
				{
					std::string compilationErrors(static_cast<const char*>(
						errors->GetBufferPointer()));

					errors->Release();
					throw std::runtime_error(compilationErrors);
				}
			}
		}

		if (effect == nullptr)
		{
			throw std::runtime_error("Shader creation failed with an unknown error. (Does " + path + " exist?)");
		}

		effect->SetTechnique("Main");
		shaders[options] = effect;
		return effect;
	}

	static void begin()
	{
		++drawing;
	}
	static void end()
	{
		if (d3d::effect == nullptr || drawing > 0 && --drawing < 1)
		{
			drawing = 0;
			d3d::do_effect = false;
		}
	}

	static void endEffect()
	{
		if (d3d::effect != nullptr && beganEffect)
		{
			d3d::effect->EndPass();
			d3d::effect->End();
		}

		beganEffect = false;
	}

	static void startEffect()
	{
		if (!d3d::do_effect || d3d::effect == nullptr
			|| d3d::do_effect && !drawing)
		{
			endEffect();
			return;
		}

		bool changes = false;

		if (sanitize(shader_options) && shader_options != last_options)
		{
			endEffect();
			changes = true;

			last_options = shader_options;
			auto e = shaders[shader_options];
			if (e == nullptr)
			{
				try
				{
					e = compileShader(shader_options);
				}
				catch (std::exception& ex)
				{
					d3d::effect = nullptr;
					endEffect();
					MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
					return;
				}
			}

			d3d::effect = e;
		}

		for (auto& it : param::parameters)
		{
			if (it->Commit(d3d::effect))
			{
				changes = true;
			}
		}

		if (beganEffect)
		{
			if (changes)
			{
				d3d::effect->CommitChanges();
			}
		}
		else
		{
			UINT passes = 0;
			if (FAILED(d3d::effect->Begin(&passes, 0)))
			{
				throw std::runtime_error("Failed to begin shader!");
			}

			if (passes > 1)
			{
				throw std::runtime_error("Multi-pass shaders are not supported.");
			}

			if (FAILED(d3d::effect->BeginPass(0)))
			{
				throw std::runtime_error("Failed to begin pass!");
			}

			beganEffect = true;
		}
	}

	static void setLightParameters()
	{
		if (!LanternInstance::UsePalette() || d3d::effect == nullptr)
		{
			return;
		}

		D3DLIGHT9 light;
		d3d::device->GetLight(0, &light);
		param::LightDirection = -*(D3DXVECTOR3*)&light.Direction;
	}

	static void hookVtbl()
	{
		enum
		{
			IndexOf_DrawPrimitive = 81,
			IndexOf_DrawIndexedPrimitive,
			IndexOf_DrawPrimitiveUP,
			IndexOf_DrawIndexedPrimitiveUP
		};

		auto vtbl = (void**)(*(void**)d3d::device);

	#define HOOK(NAME) \
	MH_CreateHook(vtbl[IndexOf_ ## NAME], NAME ## _r, (LPVOID*)& ## NAME ## _t)

		HOOK(DrawPrimitive);
		HOOK(DrawIndexedPrimitive);
		HOOK(DrawPrimitiveUP);
		HOOK(DrawIndexedPrimitiveUP);

		MH_EnableHook(MH_ALL_HOOKS);
	}

#pragma region Trampolines

	template<typename T, typename... Args>
	static void runTrampoline(const T& original, Args... args)
	{
		begin();
		original(args...);
		end();
	}

	static void drawPolyBuff(PolyBuff* _this, D3DPRIMITIVETYPE type)
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
				Direct3D_Device->SetRenderState(D3DRS_CULLMODE, args->CullMode);
				cullmode = args->CullMode;
			}

			Direct3D_Device->DrawPrimitive(type, args->StartVertex, args->PrimitiveCount);
			++args;
		}

		_this->LockCount = 0;
	}

	static void __cdecl sub_77EAD0_r(void* a1, int a2, int a3)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(sub_77EAD0), a1, a2, a3);
		end();
	}

	static void __cdecl sub_77EBA0_r(void* a1, int a2, int a3)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(sub_77EBA0), a1, a2, a3);
		end();
	}

	static void __cdecl njDrawModel_SADX_r(NJS_MODEL_SADX* a1)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);
		end();
	}

	static void __cdecl njDrawModel_SADX_B_r(NJS_MODEL_SADX* a1)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX_B), a1);
		end();
	}

	static void __fastcall PolyBuff_DrawTriangleStrip_r(PolyBuff* _this)
	{
		begin();
		drawPolyBuff(_this, D3DPT_TRIANGLESTRIP);
		end();
	}

	static void __fastcall PolyBuff_DrawTriangleList_r(PolyBuff* _this)
	{
		begin();
		drawPolyBuff(_this, D3DPT_TRIANGLELIST);
		end();
	}

	static void __fastcall CreateDirect3DDevice_r(int a1, int behavior, int type)
	{
		TARGET_DYNAMIC(CreateDirect3DDevice)(a1, behavior, type);
		if (Direct3D_Device != nullptr && !initialized)
		{
			d3d::device = Direct3D_Device->GetProxyInterface();
			initialized = true;
			d3d::LoadShader();
			hookVtbl();
		}
	}

	static void __cdecl Direct3D_SetWorldTransform_r()
	{
		TARGET_DYNAMIC(Direct3D_SetWorldTransform)();

		if (!LanternInstance::UsePalette() || d3d::effect == nullptr)
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

		if (d3d::effect == nullptr)
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

		if (d3d::effect != nullptr && invalid)
		{
			param::ProjectionMatrix = _ProjectionMatrix * TransformationMatrix;
		}
	}

	static void __cdecl Direct3D_PerformLighting_r(int type)
	{
		auto target = TARGET_DYNAMIC(Direct3D_PerformLighting);

		if (d3d::effect == nullptr || !LanternInstance::UsePalette())
		{
			target(type);
			return;
		}

		// This specifically force light type 0 to prevent
		// the light direction from being overwritten.
		target(0);
		d3d::SetShaderOptions(d3d::UseLight, true);

		if (type != globals::light_type)
		{
			setLightParameters();
		}

		globals::palettes.SetPalettes(type, globals::no_specular ? NJD_FLAG_IGNORE_SPECULAR : 0);
	}


#define D3DORIG(NAME) \
	NAME ## _t

	static HRESULT __stdcall DrawPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT StartVertex,
		UINT PrimitiveCount)
	{
		startEffect();
		auto result = D3DORIG(DrawPrimitive)(_this, PrimitiveType, StartVertex, PrimitiveCount);
		endEffect();
		return result;
	}
	static HRESULT __stdcall DrawIndexedPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		INT BaseVertexIndex,
		UINT MinVertexIndex,
		UINT NumVertices,
		UINT startIndex,
		UINT primCount)
	{
		startEffect();
		auto result = D3DORIG(DrawIndexedPrimitive)(_this, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		endEffect();
		return result;
	}
	static HRESULT __stdcall DrawPrimitiveUP_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT PrimitiveCount,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride)
	{
		startEffect();
		auto result = D3DORIG(DrawPrimitiveUP)(_this, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
		endEffect();
		return result;
	}
	static HRESULT __stdcall DrawIndexedPrimitiveUP_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT MinVertexIndex,
		UINT NumVertices,
		UINT PrimitiveCount,
		CONST void* pIndexData,
		D3DFORMAT IndexDataFormat,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride)
	{
		startEffect();
		auto result = D3DORIG(DrawIndexedPrimitiveUP)(_this, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
		endEffect();
		return result;
	}

	static void __stdcall DrawMeshSetBuffer_c(MeshSetBuffer* buffer)
	{
		if (!buffer->FVF)
		{
			return;
		}

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

	static const auto loc_77EF09 = (void*)0x0077EF09;
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
		if (d3d::effect != nullptr)
		{
			param::ProjectionMatrix = *matrix;
		}

		return Direct3D_Device->SetTransform(type, matrix);
	}
#pragma endregion
}

namespace d3d
{
	IDirect3DDevice9* device = nullptr;
	Effect effect = nullptr;
	bool do_effect = false;

	void LoadShader()
	{
		if (!local::initialized)
		{
			return;
		}

		local::releaseShaders();

		try
		{
			effect = local::compileShader(local::DEFAULT_OPTIONS);
			local::updateHandles();
		}
		catch (std::exception& ex)
		{
			effect = nullptr;
			MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
		}
	}

	void SetShaderOptions(Uint32 options, bool add)
	{
		if (add)
		{
			local::shader_options |= options;
		}
		else
		{
			local::shader_options &= ~options;
		}
	}

	void InitTrampolines()
	{
		using namespace local;

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
}

// These exports are for the window resize branch of the mod loader.
extern "C"
{
	using namespace local;

	EXPORT void __cdecl OnRenderDeviceLost()
	{
		end();

		for (auto& e : shaders)
		{
			if (e != nullptr)
			{
				e->OnLostDevice();
			}
		}
	}

	EXPORT void __cdecl OnRenderDeviceReset()
	{
		for (auto& e : shaders)
		{
			if (e != nullptr)
			{
				e->OnResetDevice();
			}
		}

		updateHandles();
	}

	EXPORT void __cdecl OnExit()
	{
		releaseParameters();
		releaseShaders();
	}
}
