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
#include "EffectParameter.h"
#include <algorithm>

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

#ifdef USE_OIT
	EffectParameter<Texture>     AlphaDepth("AlphaDepth", nullptr);
	EffectParameter<Texture>     OpaqueDepth("OpaqueDepth", nullptr);
	EffectParameter<int>         SourceBlend("SourceBlend", 0);
	EffectParameter<int>         DestinationBlend("DestinationBlend", 0);
	EffectParameter<D3DXVECTOR2> ViewPort("ViewPort", {});
	EffectParameter<float>       DrawDistance("DrawDistance", 0.0f);
	EffectParameter<float>       DepthOverride("DepthOverride", 0.0f);
#endif

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

	#ifdef USE_OIT
		&AlphaDepth,
		&OpaqueDepth,
		&SourceBlend,
		&DestinationBlend,
		&ViewPort,
		&DrawDistance,
		&DepthOverride,
	#endif

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

	static HRESULT __stdcall SetRenderState_r(IDirect3DDevice9* _this, D3DRENDERSTATETYPE State, DWORD Value);
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

	static decltype(SetRenderState_r)*         SetRenderState_t         = nullptr;
	static decltype(SetTexture_r)*             SetTexture_t             = nullptr;
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

#ifdef USE_OIT
	static const int numPasses                = 4;
	static Effect blender                     = nullptr;
	static Texture depthUnits[2]              = {};
	static Texture renderLayers[numPasses]    = {};
	static Texture blendModeLayers[numPasses] = {};
	static Texture depthBuffer                = nullptr;
	static Surface depthSurface               = nullptr;
	static Texture backBuffers[2]             = {};
	static Surface backBufferSurface          = nullptr;
	static Surface origRenderTarget           = nullptr;
	static bool peeling                       = false;

	DataPointer(D3DPRESENT_PARAMETERS, PresentParameters, 0x03D0FDC0);
#endif

	DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);
	DataPointer(D3DXMATRIX, InverseViewMatrix, 0x0389D358);
	DataPointer(D3DXMATRIX, TransformationMatrix, 0x03D0FD80);
	DataPointer(D3DXMATRIX, ViewMatrix, 0x0389D398);
	DataPointer(D3DXMATRIX, WorldMatrix, 0x03D12900);
	DataPointer(D3DXMATRIX, _ProjectionMatrix, 0x03D129C0);
	DataPointer(int, TransformAndViewportInvalid, 0x03D0FD1C);

	static auto sanitize(Uint32& options)
	{
		options &= d3d::Mask;

		if (options & d3d::UseBlend && !(options & d3d::UseLight))
		{
			options &= ~d3d::UseBlend;
		}

		if (options & d3d::UseEnvMap && !(options & d3d::UseTexture))
		{
			options &= ~d3d::UseEnvMap;
		}

		return options;
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

	#ifdef USE_OIT
		blender = nullptr;
	#endif
	}

#ifdef USE_OIT
	static void createDepthTextures()
	{
		using namespace d3d;
		auto& present = PresentParameters;
		HRESULT h;

		for (auto& it : depthUnits)
		{
			h = device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
				1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)'ZTNI', D3DPOOL_DEFAULT, &it, nullptr);

			if (FAILED(h))
			{
				throw;
			}
		}

		for (auto& it : renderLayers)
		{
			h = device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
				1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &it, nullptr);

			if (FAILED(h))
			{
				throw;
			}
		}

		for (auto& it : blendModeLayers)
		{
			h = device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
				1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &it, nullptr);

			if (FAILED(h))
			{
				throw;
			}
		}

		for (auto& it : backBuffers)
		{
			h = device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
				1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &it, nullptr);

			if (FAILED(h))
			{
				throw;
			}
		}

		device->SetDepthStencilSurface(nullptr);
		depthSurface = nullptr;
		depthBuffer = nullptr;

		device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
			1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)'ZTNI', D3DPOOL_DEFAULT, &depthBuffer, nullptr);

		backBuffers[0]->GetSurfaceLevel(0, &backBufferSurface);

		device->GetRenderTarget(0, &origRenderTarget);
		device->SetRenderTarget(0, backBufferSurface);

		depthBuffer->GetSurfaceLevel(0, &depthSurface);
		device->SetDepthStencilSurface(depthSurface);

		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	}

	static void releaseDepthTextures()
	{
		for (auto& it : depthUnits)
		{
			it = nullptr;
		}

		for (auto& it : renderLayers)
		{
			it = nullptr;
		}

		for (auto& it : blendModeLayers)
		{
			it = nullptr;
		}

		for (auto& it : backBuffers)
		{
			it = nullptr;
		}

		param::AlphaDepth.Release();
		param::OpaqueDepth.Release();

		depthSurface = nullptr;
		depthBuffer = nullptr;
		backBufferSurface = nullptr;
		origRenderTarget = nullptr;
	}

#endif

	static std::string shaderOptionsString(Uint32 o)
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

			if (o & UseFog)
			{
				o &= ~UseFog;
				result << "USE_FOG";
				thing = true;
				continue;
			}

			if (o & UseBlend)
			{
				o &= ~UseBlend;
				result << "USE_BLEND";
				thing = true;
				continue;
			}

			if (o & UseLight)
			{
				o &= ~UseLight;
				result << "USE_LIGHT";
				thing = true;
				continue;
			}

			if (o & UseAlpha)
			{
				o &= ~UseAlpha;
				result << "USE_ALPHA";
				thing = true;
				continue;
			}

			if (o & UseEnvMap)
			{
				o &= ~UseEnvMap;
				result << "USE_ENVMAP";
				thing = true;
				continue;
			}

			if (o & UseTexture)
			{
				o &= ~UseTexture;
				result << "USE_TEXTURE";
				thing = true;
				continue;
			}

			if (o & UseOit)
			{
				o &= ~UseOit;
				result << "USE_OIT";
				thing = true;
				continue;
			}

			break;
		}

		return result.str();
	}

	static Effect compileShader(Uint32 options)
	{
		PrintDebug("[lantern] Compiling shader #%02d: %08X (%s)\n", ++shaderCount, options,
			shaderOptionsString(options).c_str());

		if (pool == nullptr)
		{
			if (FAILED(D3DXCreateEffectPool(&pool)))
			{
				throw std::runtime_error("Failed to create effect pool?!");
			}
		}

		macros.clear();
		auto o = sanitize(options);

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

			if (o & UseOit)
			{
				o &= ~UseOit;
				macros.push_back({ "USE_OIT", "1" });
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

	static int startEffect()
	{
		if (!d3d::do_effect && !peeling || d3d::effect == nullptr
			|| d3d::do_effect && !drawing)
		{
			endEffect();
			return 1;
		}

		bool changes = false;

		// The value here is copied so that UseBlend can be safely removed
		// when possible without permanently removing it. It's required by
		// Sky Deck, and it's only added to the flags once on stage load.
		auto options = shader_options;

		if (d3d::effect != blender)
		{
			if (sanitize(options) && options != last_options)
			{
				endEffect();
				changes = true;

				last_options = options;
				auto e = shaders[options];

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
						return -1;
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

			if (FAILED(d3d::effect->BeginPass(0)))
			{
				throw std::runtime_error("Failed to begin pass!");
			}

			beganEffect = true;
			return passes;
		}

		return 1;
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
			IndexOf_SetRenderState = 57,
			IndexOf_SetTexture = 65,
			IndexOf_DrawPrimitive = 81,
			IndexOf_DrawIndexedPrimitive,
			IndexOf_DrawPrimitiveUP,
			IndexOf_DrawIndexedPrimitiveUP
		};

		auto vtbl = (void**)(*(void**)d3d::device);

	#define HOOK(NAME) \
	MH_CreateHook(vtbl[IndexOf_ ## NAME], NAME ## _r, (LPVOID*)& ## NAME ## _t)

		HOOK(SetRenderState);
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
			createDepthTextures();
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

		param::DrawDistance = farPlane;

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
		SetShaderOptions(d3d::UseLight, true);

		if (type != globals::light_type)
		{
			setLightParameters();
		}

		globals::palettes.SetPalettes(type, globals::no_specular ? NJD_FLAG_IGNORE_SPECULAR : 0);
	}


#define D3DORIG(NAME) \
	NAME ## _t

	HRESULT __stdcall SetRenderState_r(IDirect3DDevice9* _this, D3DRENDERSTATETYPE State, DWORD Value)
	{
		switch (State)
		{
			case D3DRS_SRCBLEND:
				param::SourceBlend = Value;
				break;
			case D3DRS_DESTBLEND:
				param::DestinationBlend = Value;
				break;
			default:
				break;
		}

		return D3DORIG(SetRenderState)(_this, State, Value);
	}

	static HRESULT __stdcall SetTexture_r(IDirect3DDevice9* _this, DWORD Stage, IDirect3DBaseTexture9* pTexture)
	{
		if (!Stage)
		{
			Texture t = (IDirect3DTexture9*)pTexture;
			param::BaseTexture.Release();
			param::BaseTexture = t;
		}

		return D3DORIG(SetTexture)(_this, Stage, pTexture);
	}

	template<typename T, typename... Args>
	static HRESULT runEffect(const T& original, Args... args)
	{
		if (!d3d::do_effect)
		{
			return original(args...);
		}

		startEffect();
		auto result = original(args...);
		endEffect();
		return result;
	}


	static HRESULT __stdcall DrawPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT StartVertex,
		UINT PrimitiveCount)
	{
		return runEffect(D3DORIG(DrawPrimitive), _this, PrimitiveType, StartVertex, PrimitiveCount);
	}
	static HRESULT __stdcall DrawIndexedPrimitive_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		INT BaseVertexIndex,
		UINT MinVertexIndex,
		UINT NumVertices,
		UINT startIndex,
		UINT primCount)
	{
		return runEffect(D3DORIG(DrawIndexedPrimitive), _this, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}
	static HRESULT __stdcall DrawPrimitiveUP_r(IDirect3DDevice9* _this,
		D3DPRIMITIVETYPE PrimitiveType,
		UINT PrimitiveCount,
		CONST void* pVertexStreamZeroData,
		UINT VertexStreamZeroStride)
	{
		return runEffect(D3DORIG(DrawPrimitiveUP), _this, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
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
		return runEffect(D3DORIG(DrawIndexedPrimitiveUP),
			_this, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
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

#ifdef USE_OIT

	struct QuadVertex
	{
		static const UINT Format = D3DFVF_XYZRHW | D3DFVF_TEX1;
		D3DXVECTOR4 Position;
		D3DXVECTOR2 TexCoord;
	};

	static void DrawQuad()
	{
		const auto& present = PresentParameters;
		QuadVertex quad[4] = {};

		auto fWidth5 = present.BackBufferWidth - 0.5f;
		auto fHeight5 = present.BackBufferHeight - 0.5f;
		const float left = 0.0f;
		const float top = 0.0f;
		const float right = 1.0f;
		const float bottom = 1.0f;

		D3DXVECTOR2 v(fWidth5, fHeight5);
		param::ViewPort = v;
		blender->SetFloatArray("ViewPort", (float*)&v, 2);

		quad[0].Position = D3DXVECTOR4(-0.5f, -0.5f, 0.5f, 1.0f);
		quad[0].TexCoord = D3DXVECTOR2(left, top);

		quad[1].Position = D3DXVECTOR4(fWidth5, -0.5f, 0.5f, 1.0f);
		quad[1].TexCoord = D3DXVECTOR2(right, top);

		quad[2].Position = D3DXVECTOR4(-0.5f, fHeight5, 0.5f, 1.0f);
		quad[2].TexCoord = D3DXVECTOR2(left, bottom);

		quad[3].Position = D3DXVECTOR4(fWidth5, fHeight5, 0.5f, 1.0f);
		quad[3].TexCoord = D3DXVECTOR2(right, bottom);

		d3d::device->SetFVF(QuadVertex::Format);
		d3d::device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &quad, sizeof(QuadVertex));
	}

	static void __cdecl DrawQueuedModels_r();
	static Trampoline DrawQueuedModels_t(0x004086F0, 0x004086F6, DrawQueuedModels_r);

	struct FVFStruct_H
	{
		D3DXVECTOR3 position;
		Uint32 diffuse;
		D3DXVECTOR2 uv;
	};
	
	DataPointer(NJS_ARGB, _nj_constant_material_, 0x03D0F7F0);

	static void __cdecl AddToQueue_r(QueuedModelNode* node, float pri, int idk);
	static void __declspec(naked) AddToQueue_asm()
	{
		__asm
		{
			push [esp + 08h] // idk
			push [esp + 08h] // pri
			push esi // node

			// Call your __cdecl function here:
			call AddToQueue_r

			pop esi // node
			add esp, 8
			retn
		}
	}

	static Trampoline AddToQueue_t(0x00403E80, 0x00403E87, AddToQueue_asm);

	void __cdecl AddToQueue_o(QueuedModelNode* node, float pri, int idk)
	{
		// ReSharper disable once CppEntityNeverUsed
		auto orig = AddToQueue_t.Target();

		__asm
		{
			push idk
			push pri
			mov esi, node
			call orig
			add esp, 8
		}
	}

	std::deque<QueuedModelNode*> nodes;

	void __cdecl AddToQueue_r(QueuedModelNode* node, float pri, int idk)
	{
		if (node == nullptr)
		{
			AddToQueue_o(node, pri, idk);
			return;
		}

		node->Depth = pri;

		switch ((QueuedModelType)(node->Flags & 0xF))
		{
			case QueuedModelType_BasicModel:
				nodes.push_back(node);
				break;

			// strictly for debugging
			case QueuedModelType_Sprite3D:
				nodes.push_back(node);
				break;

			default:
				AddToQueue_o(node, pri, idk);
				break;
		}
	}

	DataPointer(Sint8, fogemulation, 0x00909EB4);
	DataPointer(bool, FogEnabled, 0x03ABDCFE);
	DataPointer(int, CurrentLightType, 0x03B12208);
	DataPointer(NJS_TEXLIST*, CurrentTexList, 0x03ABD950);

	#pragma pack(push, 8)
	struct QueuedModelPointer
	{
		QueuedModelNode Node;
		NJS_MODEL_SADX *Model;
		NJS_MATRIX Transform;
	};
	#pragma pack(pop)

	#pragma pack(push, 8)
	struct QueuedModelSprite
	{
		QueuedModelNode Node;
		void* dummy;
		NJS_SPRITE Sprite;
		NJD_SPRITE SpriteFlags;
		float SpritePri;
		NJS_MATRIX Transform;
	};
	#pragma pack(pop)

	static void renderLayerPasses()
	{
		using namespace d3d;

		auto control_3d_orig          = _nj_control_3d_flag_;
		auto attr_and_orig            = _nj_constant_attr_and_;
		auto attr_or_orig             = _nj_constant_attr_or_;
		auto currentLightType_orig    = CurrentLightType;
		auto fov_orig                 = HorizontalFOV_BAMS;
		auto negative_one             = -1;
		auto fogEnabled_orig          = FogEnabled;
		auto fov_orig_again           = HorizontalFOV_BAMS;
		auto fov_orig_yet_again       = HorizontalFOV_BAMS;
		auto fog_emulation_orig_again = fogemulation;

		ClampGlobalColorThing_Thing();

		if (CurrentLightType)
		{
			SetCurrentLightType(0);
		}

		NJS_MATRIX orig_matrix {};
		njGetMatrix(orig_matrix);

		auto _fogemulation = fogemulation;

		int passes;
		if (!(fogemulation & 2) || (passes = 2, !(fogemulation & 1)))
		{
			passes = 1;
		}

		SetShaderOptions(UseOit, true);
		param::OpaqueDepth = depthBuffer;
		peeling = true;
		do_effect = true;

		begin();

		for (int p = 0; p < passes; p++)
		{
			// TODO: loop per draw type & use [numPasses] depth buffers
			for (int i = 0; i < numPasses; i++)
			{
				int currId = i % 2;
				int lastId = (i + 1) % 2;

				Texture currUnit = depthUnits[currId];
				Texture lastUnit = depthUnits[lastId];

				if (!i)
				{
					Surface temp;
					lastUnit->GetSurfaceLevel(0, &temp);
					device->SetDepthStencilSurface(temp);
					device->Clear(0, nullptr, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
				}

				Surface unit      = nullptr;
				Surface target    = nullptr;
				Surface blendmode = nullptr;

				currUnit->GetSurfaceLevel(0, &unit);
				device->SetDepthStencilSurface(unit);

				renderLayers[i]->GetSurfaceLevel(0, &target);
				device->SetRenderTarget(0, target);

				blendModeLayers[i]->GetSurfaceLevel(0, &blendmode);
				device->SetRenderTarget(1, blendmode);

				device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

				// Always use LESS comparison with the native d3d depth test.
				device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

				// Set the depth buffer to test against if above stuff.
				param::AlphaDepth = lastUnit;

				for (auto& it : nodes)
				{
					auto flags = it->Flags;
					auto _flags = flags;

					_nj_control_3d_flag_   = it->Control3D;
					_nj_constant_attr_and_ = it->ConstantAndAttr;
					_nj_constant_attr_or_  = it->ConstantOrAttr | NJD_FLAG_DOUBLE_SIDE;

					auto v5 = it->TexNum;
					auto v6 = _flags >> 6;

					#define SHIBYTE(x)   (*((int8_t*)&(x)+1))
					if (_flags & QueuedModelFlags_FogEnabled && SHIBYTE(v5) >= 0)
					{
						if (_fogemulation & 2 && _fogemulation & 1 && passes != 1)
						{
							continue;
						}

						if (!negative_one)
						{
							ToggleStageFog();
						}

						negative_one = 1;
					}
					else if (!(_fogemulation & 2) || !(_fogemulation & 1) || passes != 1)
					{
						if (negative_one)
						{
							DisableFog();
						}
						negative_one = 0;
					}

					if (v6 != -1)
					{
						if (SHIBYTE(v5) < 0)
						{
							_nj_constant_attr_or_ |= NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_IGNORE_SPECULAR;
							v6 = 0;
						}
						if (v6 != CurrentLightType)
						{
							SetCurrentLightType(v6);
						}
						ScaleVectorThing_Restore();
					}

					auto texlist = it->TexList;

					CurrentTexList = texlist;

					if (texlist && texlist->nbTexture)
					{
						Direct3D_SetTexList(texlist);
					}

					fov_orig = fov_orig_yet_again;
					
					Direct3D_SetZFunc(1u);
					Direct3D_EnableZWrite(1u);

					njSetConstantMaterial(&it->Color);
					njColorBlendingMode_(0, (NJD_COLOR_BLENDING)(it->BlendMode & 0xF));
					njColorBlendingMode_(NJD_DESTINATION_COLOR, (NJD_COLOR_BLENDING)(it->BlendMode >> 4) & 0xF);

					if (fogemulation & 2 && fogemulation & 1)
					{
						fogemulation &= ~2u;
					}

					switch (flags & 0x0F)
					{
						default:
							break;

						case QueuedModelType_BasicModel:
						{
							if (!texlist)
							{
								break;
							}

							auto inst = (QueuedModelPointer*)it;
							auto model = inst->Model;

							if (fov_orig_again != fov_orig)
							{
								fov_orig = fov_orig_again;
								fov_orig_yet_again = fov_orig_again;
								njSetScreenDist(fov_orig_again);
							}

							njSetMatrix(nullptr, inst->Transform);

							if ((unsigned int)model >= 0x100000)
							{
								DrawModel_ResetRenderFlags(model);
							}
							break;
						}

						case QueuedModelType_Sprite3D:
						{
							auto inst = (QueuedModelSprite*)it;

							auto tlist = inst->Sprite.tlist;
							if (!tlist)
							{
								break;
							}

							if (fov_orig_again != fov_orig)
							{
								fov_orig = fov_orig_again;
								fov_orig_yet_again = fov_orig_again;
								njSetScreenDist(fov_orig_again);
							}

							if (inst->SpriteFlags & NJD_SPRITE_SCALE)
							{
								param::DepthOverride = -it->Depth;
							}

							njSetMatrix(nullptr, inst->Transform);
							njDrawSprite3D_DrawNow(&inst->Sprite, it->TexNum & 0x7FFF, inst->SpriteFlags);
							param::DepthOverride = 0.0f;
							break;
						}
					}

					fogemulation = fog_emulation_orig_again;
				}

				currUnit  = nullptr;
				lastUnit  = nullptr;
				unit      = nullptr;
				target    = nullptr;
				blendmode = nullptr;
			}
		}

		end();

		peeling = false;
		param::OpaqueDepth = nullptr;
		param::AlphaDepth = nullptr;
		SetShaderOptions(UseOit, false);
		device->SetRenderTarget(1, nullptr);

		njSetMatrix(nullptr, orig_matrix);
		CurrentTexList = nullptr;
		SetMaterialAndSpriteColor((NJS_ARGB*)0x03AB9864); // &GlobalSpriteColor
		SetCurrentLightType_B(currentLightType_orig);
		ScaleVectorThing_Restore();
		Direct3D_SetZFunc(1u);
		Direct3D_EnableZWrite(1u);

		_nj_control_3d_flag_   = control_3d_orig;
		_nj_constant_attr_and_ = attr_and_orig;
		_nj_constant_attr_or_  = attr_or_orig;

		if (fov_orig != fov_orig_again)
		{
			njSetScreenDist(fov_orig_again);
		}

		if (fogEnabled_orig)
		{
			ToggleStageFog();
		}
		else
		{
			DisableFog();
		}
	}

	static void renderBackBuffer()
	{
		using namespace d3d;
		DWORD zenable, lighting, alphablendenable, alphatestenable,
			srcblend, dstblend;

		device->GetRenderState(D3DRS_ZENABLE, &zenable);
		device->GetRenderState(D3DRS_LIGHTING, &lighting);
		device->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphablendenable);
		device->GetRenderState(D3DRS_ALPHATESTENABLE, &alphatestenable);
		device->GetRenderState(D3DRS_SRCBLEND, &srcblend);
		device->GetRenderState(D3DRS_DESTBLEND, &dstblend);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		device->SetRenderState(D3DRS_ZENABLE, false);
		device->SetRenderState(D3DRS_LIGHTING, false);

		device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTOP_SELECTARG1);
		device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTOP_DISABLE);
		device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTOP_SELECTARG1);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTOP_DISABLE);

		auto pad = ControllerPointers[0];

		unsigned int passes;
		blender->Begin(&passes, 0);
		auto _effect = effect;
		effect = blender;

		do_effect = true;

		begin();

		int lastId = 0;
		for (int i = numPasses; i > 0; i--)
		{
			int currId = i % 2;
			lastId = (i + 1) % 2;

			blender->SetTexture("BackBuffer", backBuffers[currId]);
			blender->SetTexture("AlphaLayer", renderLayers[i - 1]);
			blender->SetTexture("BlendLayer", blendModeLayers[i - 1]);

			Surface surface = nullptr;
			backBuffers[lastId]->GetSurfaceLevel(0, &surface);
			device->SetRenderTarget(0, surface);
			device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0.0f, 0);

			DrawQuad();

			surface = nullptr;

			if (pad && pad->PressedButtons & Buttons_Right)
			{
				std::string path = "layer" + std::to_string(i) + ".png";
				D3DXSaveTextureToFileA(path.c_str(), D3DXIFF_PNG, renderLayers[i - 1], nullptr);

				path = "blend" + std::to_string(i) + ".png";
				D3DXSaveTextureToFileA(path.c_str(), D3DXIFF_PNG, blendModeLayers[i - 1], nullptr);
			}
		}

		end();

		effect = _effect;

		blender->SetTexture("BackBuffer", nullptr);
		blender->SetTexture("AlphaLayer", nullptr);
		blender->SetTexture("BlendLayer", nullptr);
		blender->EndPass();
		blender->End();

		device->SetRenderTarget(0, origRenderTarget);
		device->SetDepthStencilSurface(depthSurface);
		device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);

		if (pad && pad->PressedButtons & Buttons_Right)
		{
			D3DXSaveTextureToFileA("backbuffer.png", D3DXIFF_PNG, backBuffers[lastId], nullptr);
		}

		do_effect = false;

		device->SetTexture(0, backBuffers[lastId]);
		DrawQuad();
		device->SetTexture(0, nullptr);

		device->SetRenderState(D3DRS_ZENABLE, zenable);
		device->SetRenderState(D3DRS_LIGHTING, lighting);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, alphablendenable);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, alphatestenable);
		device->SetRenderState(D3DRS_SRCBLEND, srcblend);
		device->SetRenderState(D3DRS_DESTBLEND, dstblend);

		// draw hud and stuff
		auto draw = TARGET_STATIC(DrawQueuedModels);
		draw();

		device->SetRenderTarget(0, backBufferSurface);
	}

	bool node_sort(QueuedModelNode* a, QueuedModelNode* b)
	{
		return a->TexList != b->TexList
		&& a->TexNum > b->TexNum
		&& a->Flags != b->Flags
		&& a->BlendMode != b->BlendMode
		&& a->Depth < b->Depth;
	}

	static void __cdecl DrawQueuedModels_r()
	{
		if (!nodes.empty())
		{
			sort(nodes.begin(), nodes.end(), &node_sort);
			renderLayerPasses();
			nodes.clear();
		}

		renderBackBuffer();
	}

	DataArray(D3DBLEND, BlendModes, 0x0088AD6C, 12);
	static void __cdecl njColorBlendingMode__r(Int target, Int mode);
	static Trampoline njColorBlendingMode__t(0x0077EC60, 0x0077EC66, njColorBlendingMode__r);
	static void __cdecl njColorBlendingMode__r(Int target, Int mode)
	{
		using namespace d3d;
		/*if (peeling)
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		}
		else
		{*/
			TARGET_STATIC(njColorBlendingMode_)(target, mode);
		//}

		if (mode)
		{
			if (mode == 1)
			{
				param::SourceBlend = D3DBLEND_SRCALPHA;
				param::DestinationBlend = D3DBLEND_INVSRCALPHA;
			}
			else
			{
				if (!target)
				{
					param::SourceBlend = BlendModes[mode];
				}
				else
				{
					param::DestinationBlend = BlendModes[mode];
				}
			}
		}
		else
		{
			param::SourceBlend = D3DBLEND_INVSRCALPHA;
			param::DestinationBlend = D3DBLEND_SRCALPHA;
		}
	}

	static void __fastcall njAlphaMode_r(Int mode)
	{
		using namespace d3d;
		if (mode == 0)
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
			device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		}
		else
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, !peeling);
			device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		}
	}

	static void __fastcall njTextureShadingMode_r(Int mode)
	{
		using namespace d3d;
		if (mode)
		{
			if (--mode)
			{
				if (mode == 1)
				{
					device->SetRenderState(D3DRS_ALPHABLENDENABLE, !peeling);
					device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
					device->SetRenderState(D3DRS_ALPHAREF, 16);
				}
			}
			else
			{
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, !peeling);
				device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				device->SetRenderState(D3DRS_ALPHAREF, 0);
			}
		}
		else
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
			device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			device->SetRenderState(D3DRS_ALPHAREF, 0);
		}
	}

#endif
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
			ID3DXBuffer* errors = nullptr;
			auto result = D3DXCreateEffectFromFileA(device, "blender.fx", nullptr, nullptr,
				D3DXFX_NOT_CLONEABLE | D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE,
				nullptr, &local::blender, &errors);

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

			if (local::blender == nullptr)
			{
				throw std::runtime_error("Shader creation failed with an unknown error. (Does blender.fx exist?)");
			}

			effect = local::compileShader(local::DEFAULT_OPTIONS);

		#ifdef PRECOMPILE_SHADERS
			for (Uint32 i = 1; i < ShaderOptions::Count; i++)
			{
				auto options = i;
				local::sanitize(options);
				if (options && local::shaders[options] == nullptr)
				{
					local::compileShader(options);
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

	#ifdef USE_OIT
		// HACK: DIRTY HACKS
		//WriteJump(Direct3D_EnableZWrite, Direct3D_EnableZWrite_r);
		//WriteJump((void*)0x0077ED00, Direct3D_SetZFunc_r);
		WriteJump((void*)0x00791940, njAlphaMode_r);
		WriteJump((void*)0x00791990, njTextureShadingMode_r);
	#endif
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

		blender->OnLostDevice();
		releaseDepthTextures();
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

		blender->OnResetDevice();

		createDepthTextures();
		updateHandles();
	}

	EXPORT void __cdecl OnExit()
	{
		releaseDepthTextures();
		releaseParameters();
		releaseShaders();
	}
}
