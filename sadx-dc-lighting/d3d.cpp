#include "stdafx.h"

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXFunctions.h>
#include <Trampoline.h>

// MinHook
#include <MinHook.h>

// Standard library
#include <sstream>
#include <vector>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"
#include "../include/lanternapi.h"
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
	EffectParameter<bool>        AllowVertexColor("AllowVertexColor", true);
	EffectParameter<bool>        ForceDefaultDiffuse("ForceDefaultDiffuse", false);

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
		&AllowVertexColor,
		&ForceDefaultDiffuse,

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
	static Trampoline* njDrawModel_SADX_Dynamic_t         = nullptr;
	static Trampoline* Direct3D_SetProjectionMatrix_t     = nullptr;
	static Trampoline* Direct3D_SetViewportAndTransform_t = nullptr;
	static Trampoline* Direct3D_SetWorldTransform_t       = nullptr;
	static Trampoline* CreateDirect3DDevice_t             = nullptr;
	static Trampoline* PolyBuff_DrawTriangleStrip_t       = nullptr;
	static Trampoline* PolyBuff_DrawTriangleList_t        = nullptr;

	static HRESULT __stdcall SetTexture_r(IDirect3DDevice9* _this, DWORD Stage, IDirect3DBaseTexture9* pTexture);
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

	static decltype(SetTexture_r)*             SetTexture_t             = nullptr;
	static decltype(DrawPrimitive_r)*          DrawPrimitive_t          = nullptr;
	static decltype(DrawIndexedPrimitive_r)*   DrawIndexedPrimitive_t   = nullptr;
	static decltype(DrawPrimitiveUP_r)*        DrawPrimitiveUP_t        = nullptr;
	static decltype(DrawIndexedPrimitiveUP_r)* DrawIndexedPrimitiveUP_t = nullptr;

	constexpr auto DEFAULT_FLAGS = ShaderFlags_Alpha | ShaderFlags_Fog | ShaderFlags_Light | ShaderFlags_Texture;

	static Uint32 shader_flags = DEFAULT_FLAGS;
	static Uint32 last_flags = DEFAULT_FLAGS;

	static std::vector<Uint8> shaderFile;
	static Uint32 shaderCount = 0;
	static Effect shaders[ShaderFlags_Count] = {};
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

	static auto sanitize(Uint32& flags)
	{
		flags &= ShaderFlags_Mask;

		if (flags & ShaderFlags_Blend && !(flags & ShaderFlags_Light))
		{
			flags &= ~ShaderFlags_Blend;
		}

		if (flags & ShaderFlags_EnvMap && !(flags & ShaderFlags_Texture))
		{
			flags &= ~ShaderFlags_EnvMap;
		}

		return flags;
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

	static std::string shaderFlagsString(Uint32 o)
	{
		std::stringstream result;

		bool thing = false;
		while (o != 0)
		{
			using namespace d3d;

			if (thing)
			{
				result << " | ";
			}

			if (o & ShaderFlags_Fog)
			{
				o &= ~ShaderFlags_Fog;
				result << "USE_FOG";
				thing = true;
				continue;
			}

			if (o & ShaderFlags_Blend)
			{
				o &= ~ShaderFlags_Blend;
				result << "USE_BLEND";
				thing = true;
				continue;
			}

			if (o & ShaderFlags_Light)
			{
				o &= ~ShaderFlags_Light;
				result << "USE_LIGHT";
				thing = true;
				continue;
			}

			if (o & ShaderFlags_Alpha)
			{
				o &= ~ShaderFlags_Alpha;
				result << "USE_ALPHA";
				thing = true;
				continue;
			}

			if (o & ShaderFlags_EnvMap)
			{
				o &= ~ShaderFlags_EnvMap;
				result << "USE_ENVMAP";
				thing = true;
				continue;
			}

			if (o & ShaderFlags_Texture)
			{
				o &= ~ShaderFlags_Texture;
				result << "USE_TEXTURE";
				thing = true;
				continue;
			}

			break;
		}

		return result.str();
	}

	static Effect compileShader(Uint32 flags)
	{
		PrintDebug("[lantern] Compiling shader #%02d: %08X (%s)\n", ++shaderCount, flags,
			shaderFlagsString(flags).c_str());

		if (pool == nullptr)
		{
			if (FAILED(D3DXCreateEffectPool(&pool)))
			{
				throw std::runtime_error("Failed to create effect pool?!");
			}
		}

		macros.clear();
		auto o = sanitize(flags);

		while (o != 0)
		{
			using namespace d3d;

			if (o & ShaderFlags_Texture)
			{
				o &= ~ShaderFlags_Texture;
				macros.push_back({ "USE_TEXTURE", "1" });
				continue;
			}

			if (o & ShaderFlags_EnvMap)
			{
				o &= ~ShaderFlags_EnvMap;
				macros.push_back({ "USE_ENVMAP", "1" });
				continue;
			}

			if (o & ShaderFlags_Light)
			{
				o &= ~ShaderFlags_Light;
				macros.push_back({ "USE_LIGHT", "1" });
				continue;
			}

			if (o & ShaderFlags_Blend)
			{
				o &= ~ShaderFlags_Blend;
				macros.push_back({ "USE_BLEND", "1" });
				continue;
			}

			if (o & ShaderFlags_Alpha)
			{
				o &= ~ShaderFlags_Alpha;
				macros.push_back({ "USE_ALPHA", "1" });
				continue;
			}

			if (o & ShaderFlags_Fog)
			{
				o &= ~ShaderFlags_Fog;
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
		shaders[flags] = effect;
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

			if (LanternInstance::diffuse_override_temp)
			{
				LanternInstance::diffuse_override = false;
			}

			if (LanternInstance::specular_override_temp)
			{
				LanternInstance::specular_override = false;
			}

			param::ForceDefaultDiffuse = false;
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

		// The value here is copied so that UseBlend can be safely removed
		// when possible without permanently removing it. It's required by
		// Sky Deck, and it's only added to the flags once on stage load.
		auto flags = shader_flags;

		if (sanitize(flags) && flags != last_flags)
		{
			endEffect();
			changes = true;

			last_flags = flags;
			auto e = shaders[flags];
			if (e == nullptr)
			{
				try
				{
					e = compileShader(flags);
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

		if (!IEffectParameter::values_assigned.empty())
		{
			for (auto& it : IEffectParameter::values_assigned)
			{
				if (it->Commit(d3d::effect))
				{
					changes = true;
				}
			}

			IEffectParameter::values_assigned.clear();
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
			IndexOf_SetTexture = 65,
			IndexOf_DrawPrimitive = 81,
			IndexOf_DrawIndexedPrimitive,
			IndexOf_DrawPrimitiveUP,
			IndexOf_DrawIndexedPrimitiveUP
		};

		auto vtbl = (void**)(*(void**)d3d::device);

	#define HOOK(NAME) \
	MH_CreateHook(vtbl[IndexOf_ ## NAME], NAME ## _r, (LPVOID*)& ## NAME ## _t)

		HOOK(SetTexture);
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

		if (a1 && a1->nbMat && a1->mats)
		{
			globals::first_material = true;

			auto _control_3d = _nj_control_3d_flag_;
			auto _attr_or = _nj_constant_attr_or_;
			auto _attr_and = _nj_constant_attr_and_;

			runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);

			_nj_control_3d_flag_ = _control_3d;
			_nj_constant_attr_and_ = _attr_and;
			_nj_constant_attr_or_ = _attr_or;
		}
		else
		{
			runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);
		}

		end();
	}

	static void __cdecl njDrawModel_SADX_Dynamic_r(NJS_MODEL_SADX* a1)
	{
		begin();

		if (a1 && a1->nbMat && a1->mats)
		{
			globals::first_material = true;

			auto _control_3d = _nj_control_3d_flag_;
			auto _attr_or = _nj_constant_attr_or_;
			auto _attr_and = _nj_constant_attr_and_;

			runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX_Dynamic), a1);

			_nj_control_3d_flag_ = _control_3d;
			_nj_constant_attr_and_ = _attr_and;
			_nj_constant_attr_or_ = _attr_or;
		}
		else
		{
			runTrampoline(TARGET_DYNAMIC(njDrawModel_SADX_Dynamic), a1);
		}

		end();
	}

	static void __fastcall PolyBuff_DrawTriangleStrip_r(PolyBuff* _this)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(PolyBuff_DrawTriangleStrip), _this);
		end();
	}

	static void __fastcall PolyBuff_DrawTriangleList_r(PolyBuff* _this)
	{
		begin();
		runTrampoline(TARGET_DYNAMIC(PolyBuff_DrawTriangleList), _this);
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
		d3d::SetShaderFlags(ShaderFlags_Light, true);

		if (type != globals::light_type)
		{
			setLightParameters();
		}

		globals::palettes.SetPalettes(type, 0);
	}


#define D3DORIG(NAME) \
	NAME ## _t

	static HRESULT __stdcall SetTexture_r(IDirect3DDevice9* _this, DWORD Stage, IDirect3DBaseTexture9* pTexture)
	{
		if (!Stage)
		{
			param::BaseTexture = (IDirect3DTexture9*)pTexture;
		}

		return D3DORIG(SetTexture)(_this, Stage, pTexture);
	}

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
			effect = local::compileShader(local::DEFAULT_FLAGS);

		#ifdef PRECOMPILE_SHADERS
			for (Uint32 i = 1; i < ShaderFlags_Count; i++)
			{
				auto flags = i;
				local::sanitize(flags);
				if (flags && local::shaders[flags] == nullptr)
				{
					local::compileShader(flags);
				}
			}
		#endif

			local::updateHandles();
		}
		catch (std::exception& ex)
		{
			effect = nullptr;
			MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
		}
	}

	void SetShaderFlags(Uint32 flags, bool add)
	{
		if (add)
		{
			local::shader_flags |= flags;
		}
		else
		{
			local::shader_flags &= ~flags;
		}
	}

	void InitTrampolines()
	{
		using namespace local;

		Direct3D_PerformLighting_t         = new Trampoline(0x00412420, 0x00412426, Direct3D_PerformLighting_r);
		sub_77EAD0_t                       = new Trampoline(0x0077EAD0, 0x0077EAD7, sub_77EAD0_r);
		sub_77EBA0_t                       = new Trampoline(0x0077EBA0, 0x0077EBA5, sub_77EBA0_r);
		njDrawModel_SADX_t                 = new Trampoline(0x0077EDA0, 0x0077EDAA, njDrawModel_SADX_r);
		njDrawModel_SADX_Dynamic_t         = new Trampoline(0x00784AE0, 0x00784AE5, njDrawModel_SADX_Dynamic_r);
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
		WriteData<8>((void*)0x00403234, 0x90i8);
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
