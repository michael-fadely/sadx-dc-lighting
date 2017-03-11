#include "stdafx.h"

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXModLoader/SADXFunctions.h>
#include <Trampoline.h>

// Standard library
#include <vector>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "lantern.h"
#include "EffectParameter.h"
#include "modelqueue.h"

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

constexpr auto DEFAULT_OPTIONS = d3d::UseAlpha | d3d::UseFog | d3d::UseLight | d3d::UseTexture;

static Uint32 shader_options = DEFAULT_OPTIONS;
static Uint32 last_options = DEFAULT_OPTIONS;

static std::vector<Uint8> shaderFile;
static Uint32 shaderCount = 0;
static Effect shaders[d3d::ShaderOptions::Count] = {};
static CComPtr<ID3DXEffectPool> pool = nullptr;

static bool initialized  = false;
static bool began_effect = false;
static Uint32 drawing    = 0;

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
DataPointer(D3DPRESENT_PARAMETERS8, PresentParameters, 0x03D0FDC0);

namespace d3d
{
	IDirect3DDevice9* device = nullptr;
	Effect effect = nullptr;
	bool do_effect = false;
}

namespace param
{
	EffectParameter<Texture> BaseTexture("BaseTexture", nullptr);

	EffectParameter<Texture> PaletteA("PaletteA", nullptr);
	EffectParameter<float> DiffuseIndexA("DiffuseIndexA", 0.0f);
	EffectParameter<float> SpecularIndexA("SpecularIndexA", 0.0f);

	EffectParameter<Texture> PaletteB("PaletteB", nullptr);
	EffectParameter<float> DiffuseIndexB("DiffuseIndexB", 0.0f);
	EffectParameter<float> SpecularIndexB("SpecularIndexB", 0.0f);

	EffectParameter<float> BlendFactor("BlendFactor", 0.0f);
	EffectParameter<D3DXMATRIX> WorldMatrix("WorldMatrix", {});
	EffectParameter<D3DXMATRIX> wvMatrix("wvMatrix", {});
	EffectParameter<D3DXMATRIX> ProjectionMatrix("ProjectionMatrix", {});
	EffectParameter<D3DXMATRIX> wvMatrixInvT("wvMatrixInvT", {});
	EffectParameter<D3DXMATRIX> TextureTransform("TextureTransform", {});
	EffectParameter<int> FogMode("FogMode", 0);
	EffectParameter<float> FogStart("FogStart", 0.0f);
	EffectParameter<float> FogEnd("FogEnd", 0.0f);
	EffectParameter<float> FogDensity("FogDensity", 0.0f);
	EffectParameter<D3DXCOLOR> FogColor("FogColor", {});
	EffectParameter<D3DXVECTOR3> LightDirection("LightDirection", {});
	EffectParameter<int> DiffuseSource("DiffuseSource", 0);

	EffectParameter<D3DXCOLOR> MaterialDiffuse("MaterialDiffuse", {});
	EffectParameter<float> AlphaRef("AlphaRef", 0.0f);
	EffectParameter<D3DXVECTOR3> NormalScale("NormalScale", { 1.0f, 1.0f, 1.0f });

#ifdef USE_OIT
	EffectParameter<Texture> AlphaDepth("AlphaDepth", nullptr);
	EffectParameter<Texture> OpaqueDepth("OpaqueDepth", nullptr);
	// TODO: remove
	EffectParameter<Texture> BackBuffer("BackBuffer", nullptr);
	EffectParameter<bool> AlphaDepthTest("AlphaDepthTest", false);
	EffectParameter<bool> DestBlendOne("DestBlendOne", false);
	EffectParameter<D3DXVECTOR2> ViewPort("ViewPort", {});
#endif

#ifdef USE_SL
	EffectParameter<D3DXCOLOR> MaterialSpecular("MaterialSpecular", {});
	EffectParameter<float> MaterialPower("MaterialPower", 1.0f);
	EffectParameter<SourceLight_t> SourceLight("SourceLight", {});
#endif

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
		&BackBuffer,
		&AlphaDepthTest,
		&DestBlendOne,
		&ViewPort,
#endif

#ifdef USE_SL
		&SourceLight,
		&MaterialSpecular,
		&MaterialPower,
		&UseSourceLight,
#endif
	};
}

static const int numPasses             = 4;
static Texture depthUnits[numPasses]   = {};
static Texture renderLayers[numPasses] = {};
static Texture depthBuffer             = nullptr;
static Surface depthSurface            = nullptr;
static Texture backBuffer              = nullptr;
static Surface backBufferSurface       = nullptr;
static Surface origRenderTarget        = nullptr;
static bool peeling                    = false;

static void createDepthTextures()
{
	using namespace d3d;
	auto& present = PresentParameters;

	for (auto& it : depthUnits)
	{
		device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
			1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)'ZTNI', D3DPOOL_DEFAULT, &it, nullptr);
	}

	for (auto &it : renderLayers)
	{
		device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
			1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &it, nullptr);
	}

	depthSurface = nullptr;
	device->SetDepthStencilSurface(nullptr);
	depthBuffer = nullptr;

	device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
		1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)'ZTNI', D3DPOOL_DEFAULT, &depthBuffer, nullptr);

	device->CreateTexture(present.BackBufferWidth, present.BackBufferHeight,
		1, D3DUSAGE_RENDERTARGET, present.BackBufferFormat, D3DPOOL_DEFAULT, &backBuffer, nullptr);

	backBuffer->GetSurfaceLevel(0, &backBufferSurface);

	device->GetRenderTarget(0, &origRenderTarget);
	device->SetRenderTarget(0, backBufferSurface);

	depthBuffer->GetSurfaceLevel(0, &depthSurface);
	device->SetDepthStencilSurface(depthSurface);

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

static void UpdateParameterHandles()
{
	for (auto i : param::parameters)
	{
		i->UpdateHandle(d3d::effect);
	}
}

static auto sanitize(Uint32& options)
{
	return options &= d3d::Mask;
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

	auto o = options;
	std::vector<D3DXMACRO> macros;

	while (o != 0)
	{
		if (o & d3d::UseTexture)
		{
			o &= ~d3d::UseTexture;
			macros.push_back({ "USE_TEXTURE", "1" });
			continue;
		}

		if (o & d3d::UseEnvMap)
		{
			o &= ~d3d::UseEnvMap;
			macros.push_back({ "USE_ENVMAP", "1" });
			continue;
		}

		if (o & d3d::UseLight)
		{
			o &= ~d3d::UseLight;
			macros.push_back({ "USE_LIGHT", "1" });
			continue;
		}

		if (o & d3d::UseBlend)
		{
			o &= ~d3d::UseBlend;
			macros.push_back({ "USE_BLEND", "1" });
			continue;
		}

		if (o & d3d::UseAlpha)
		{
			o &= ~d3d::UseAlpha;
			macros.push_back({ "USE_ALPHA", "1" });
			continue;
		}

		if (o & d3d::UseFog)
		{
			o &= ~d3d::UseFog;
			macros.push_back({ "USE_FOG", "1" });
			continue;
		}

		if (o & d3d::UseOit)
		{
			o &= ~d3d::UseOit;
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

using namespace d3d;

static void end();
static void begin()
{
	if (effect == nullptr)
	{
		return;
	}

	if (!do_effect || began_effect)
	{
		if ((peeling || shader_options & UseOit) && shader_options != last_options)
		{
			end();
		}
		else
		{
			return;
		}

		do_effect = true;
	}

	for (auto i : param::parameters)
	{
		i->Commit(effect);
	}

	if (sanitize(shader_options) && shader_options != last_options)
	{
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
				effect = nullptr;
				MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
				return;
			}
		}

		effect = e;
	}

	UINT passes = 0;
	if (FAILED(effect->Begin(&passes, 0)))
	{
		throw std::runtime_error("Failed to begin shader!");
	}

	if (passes > 1)
	{
		throw std::runtime_error("Multi-pass shaders are not supported.");
	}

	if (FAILED(effect->BeginPass(0)))
	{
		throw std::runtime_error("Failed to begin pass!");
	}

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
				//effect->CommitChanges();
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
	PresentParameters.EnableAutoDepthStencil = false;

	TARGET_DYNAMIC(CreateDirect3DDevice)(a1, behavior, type);
	if (Direct3D_Device != nullptr && !initialized)
	{
		device = Direct3D_Device->GetProxyInterface();
		initialized = true;
		LoadShader();
		createDepthTextures();
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

	param::WorldMatrix.Commit(effect);
	param::wvMatrix.Commit(effect);
	param::wvMatrixInvT.Commit(effect);
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
	SetShaderOptions(UseLight, true);

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
	if (effect != nullptr)
	{
		param::ProjectionMatrix = *matrix;
	}

	return device->SetTransform(type, matrix);
}

void releaseParameters()
{
	for (auto& i : param::parameters)
	{
		i->Release();
	}
}

void releaseShaders()
{
	shaderFile.clear();
	effect = nullptr;

	for (auto& e : shaders)
	{
		e = nullptr;
	}

	shaderCount = 0;
	pool = nullptr;
}

void d3d::LoadShader()
{
	if (!initialized)
	{
		return;
	}

	releaseShaders();

	try
	{
		effect = compileShader(DEFAULT_OPTIONS);
		UpdateParameterHandles();
	}
	catch (std::exception& ex)
	{
		effect = nullptr;
		MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
	}
}

void d3d::SetShaderOptions(Uint32 options, bool add)
{
	if (add)
	{
		shader_options |= options;
	}
	else
	{
		shader_options &= ~options;
	}
}

#ifdef USE_OIT

static Sint32 show_layer = 0;

const char* strings[14] = {
	"nope",
	"D3DBLEND_ZERO",
	"D3DBLEND_ONE",
	"D3DBLEND_SRCCOLOR",
	"D3DBLEND_INVSRCCOLOR",
	"D3DBLEND_SRCALPHA",
	"D3DBLEND_INVSRCALPHA",
	"D3DBLEND_DESTALPHA",
	"D3DBLEND_INVDESTALPHA",
	"D3DBLEND_DESTCOLOR",
	"D3DBLEND_INVDESTCOLOR",
	"D3DBLEND_SRCALPHASAT",
	"D3DBLEND_BOTHSRCALPHA",
	"D3DBLEND_BOTHINVSRCALPHA"
};

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

	param::ViewPort = D3DXVECTOR2(fWidth5, fHeight5);

	quad[0].Position = D3DXVECTOR4(-0.5f, -0.5f, 0.5f, 1.0f);
	quad[0].TexCoord = D3DXVECTOR2(left, top);

	quad[1].Position = D3DXVECTOR4(fWidth5, -0.5f, 0.5f, 1.0f);
	quad[1].TexCoord = D3DXVECTOR2(right, top);

	quad[2].Position = D3DXVECTOR4(-0.5f, fHeight5, 0.5f, 1.0f);
	quad[2].TexCoord = D3DXVECTOR2(left, bottom);

	quad[3].Position = D3DXVECTOR4(fWidth5, fHeight5, 0.5f, 1.0f);
	quad[3].TexCoord = D3DXVECTOR2(right, bottom);

	device->SetFVF(QuadVertex::Format);
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &quad, sizeof(QuadVertex));
}

static void __cdecl Direct3D_EnableZWrite_r(DWORD enable)
{
	if (peeling)
	{
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		return;
	}

	device->SetRenderState(D3DRS_ZWRITEENABLE, enable);
}
static void __cdecl Direct3D_SetZFunc_r(Uint8 index)
{
	if (peeling)
	{
		device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
		return;
	}

	device->SetRenderState(D3DRS_ZFUNC, index + 1);
}

static void layer_debug()
{
	auto pad = ControllerPointers[0];

	if (pad)
	{
		auto pressed = pad->PressedButtons;

		if (pressed & Buttons_Down)
		{
			++show_layer %= numPasses + 1;
		}
		else if (pressed & Buttons_Up)
		{
			if (--show_layer < 0)
			{
				show_layer = numPasses;
			}
		}
	}

	DisplayDebugStringFormatted(NJM_LOCATION(1, 1), "LAYER: %d", show_layer);
}

static bool final_blend = false;
static void __cdecl DrawQueuedModels_r();
static Trampoline DrawQueuedModels_t(0x004086F0, 0x004086F6, DrawQueuedModels_r);

static void renderLayerPasses(void(*draw)(), bool clearZ)
{
	using namespace param;

	SetShaderOptions(UseOit, true);
	OpaqueDepth = depthBuffer;
	peeling = true;
	do_effect = true;
	BackBuffer = backBuffer;

	begin();

	for (int i = 0; i < numPasses; i++)
	{
		int currId = i % numPasses;
		int lastId = currId - 1;
		if (lastId < 0)
		{
			lastId = numPasses - 1;
		}

		Texture currUnit = depthUnits[currId];
		Texture lastUnit = depthUnits[lastId];

		Surface unit   = nullptr;
		Surface target = nullptr;

		currUnit->GetSurfaceLevel(0, &unit);
		renderLayers[i]->GetSurfaceLevel(0, &target);

		device->SetDepthStencilSurface(unit);
		device->SetRenderTarget(0, target);

		// Always use LESS comparison with the native d3d depth test.
		device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
		// Clear old crap
		auto clearFlags = D3DCLEAR_TARGET;
		device->Clear(0, nullptr, clearZ ? (clearFlags | D3DCLEAR_ZBUFFER) : clearFlags, 0, 1.0f, 0);

		// Only enable manual alpha depth tests on the second pass.
		// If not, the depth test will fail and nothing will render
		// as the "last" depth buffer has not been populated yet.
		// The shader performs a manual GREATER depth test on
		// transparent things.
		AlphaDepthTest = i != 0;
		AlphaDepthTest.Commit(effect);
		// Set the depth buffer to test against if above stuff.
		AlphaDepth = lastUnit;
		AlphaDepth.Commit(effect);

		auto last = *(int*)0x3AB98AC;
		draw();
		*(int*)0x3AB98AC = last;

		currUnit = nullptr;
		lastUnit = nullptr;
		unit     = nullptr;
		target   = nullptr;

		AlphaDepth = nullptr;
		AlphaDepth.Commit(effect);
	}

	end();

	peeling = false;
	OpaqueDepth = nullptr;
	BackBuffer = nullptr;
	SetShaderOptions(UseOit, false);
}

static void renderBackBuffer(bool populate, D3DBLEND layerBlend)
{
	DWORD zenable, lighting, alphablendenable, alphatestenable,
	      srcblend, dstblend;

	device->GetRenderState(D3DRS_ZENABLE, &zenable);
	device->GetRenderState(D3DRS_LIGHTING, &lighting);
	device->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphablendenable);
	device->GetRenderState(D3DRS_ALPHATESTENABLE, &alphatestenable);
	device->GetRenderState(D3DRS_SRCBLEND, &srcblend);
	device->GetRenderState(D3DRS_DESTBLEND, &dstblend);

	device->SetRenderState(D3DRS_ZENABLE, false);
	device->SetRenderState(D3DRS_LIGHTING, false);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, false);

	device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTOP_SELECTARG1);
	device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTOP_DISABLE);
	device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTOP_SELECTARG1);
	device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTOP_DISABLE);

	device->SetRenderTarget(0, origRenderTarget);
	device->SetDepthStencilSurface(depthSurface);

	if (populate)
	{
		device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
	}

	// draw the custom backbuffer to the real backbuffer
	auto pad = ControllerPointers[0];
	if (!pad || !(pad->HeldButtons & Buttons_C))
	{
		if (populate)
		{
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetTexture(0, backBuffer);
			DrawQuad();
			device->SetTexture(0, nullptr);
		}
	}

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, layerBlend);

	for (int i = numPasses; i > 0; i--)
	{
		if (show_layer >= 1 && i != show_layer)
			continue;

		device->SetTexture(0, renderLayers[i - 1]);
		DrawQuad();
	}

	device->SetRenderState(D3DRS_ZENABLE, zenable);
	device->SetRenderState(D3DRS_LIGHTING, lighting);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, alphablendenable);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, alphatestenable);
	device->SetRenderState(D3DRS_SRCBLEND, srcblend);
	device->SetRenderState(D3DRS_DESTBLEND, dstblend);
}

static void __cdecl DrawQueuedModels_r()
{
	using namespace param;
	using namespace d3d;
	auto draw = TARGET_STATIC(DrawQueuedModels);
	device->SetRenderTarget(0, nullptr);

	renderLayerPasses(draw, true);
	renderBackBuffer(true, D3DBLEND_INVSRCALPHA);

	final_blend = true;

	renderLayerPasses(draw, false);
	renderBackBuffer(false, D3DBLEND_ONE);

	final_blend = false;

	// draw hud and stuff
	layer_debug();
	draw();

	device->SetTexture(0, nullptr);
	device->SetRenderTarget(0, backBufferSurface);
}

static NJS_MODEL_SADX* __last = nullptr;
bool __stdcall MyCoolFunction(QueuedModelNode* node, NJS_TEXLIST* texlist)
{
	switch ((QueuedModelType)(node->Type & 0xF))
	{
		case QueuedModelType_BasicModel:
		{
			if (!peeling)
			{
				return false;
			}

			if (!texlist)
			{
				return true;
			}

			auto m = (QueuedModelPointer*)node;
			auto model = m->Model;

			if (!model->nbMat || !model->nbMeshset)
			{
				return true;
			}

			constexpr auto MASK = NJD_FLAG_USE_ALPHA;

			for (int i = 0; i < model->nbMat; i++)
			{
				const NJS_MATERIAL& mat = model->mats[i];
				auto flags = mat.attrflags;

				if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
				{
					flags = _nj_constant_attr_or_ | _nj_constant_attr_and_ & flags;
				}

				if ((flags & MASK) == MASK && (flags & NJD_DA_MASK) == NJD_DA_ONE)
				{
					return final_blend;
				}
			}

			return !final_blend;
		}

		// TODO: actually handle 3D sprites (particles etc)
		case QueuedModelType_Sprite3D:
			return final_blend;

		default:
			return !peeling;
	}
}

const auto continue_draw = (void*)0x00408986;
const auto skip_draw = (void*)0x00408F17;
void __declspec(naked) saveyourself()
{
	__asm
	{
		call njColorBlendingMode_
		push ebx
		push esi
		call MyCoolFunction
		test al, al
		jnz  wangis

		// skip the type evaluation & drawing
		// (also correct the stack from all those function calls)
		add  esp, 14h
		jmp	 skip_draw

		// continue to type evaluation & draw
	wangis:
		jmp  continue_draw
	}
}

#endif

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


	// HACK: DIRTY HACKS
	WriteJump(Direct3D_EnableZWrite, Direct3D_EnableZWrite_r);
	WriteJump((void*)0x0077ED00, Direct3D_SetZFunc_r);
	WriteJump((void*)0x00408981, saveyourself);

	WriteJump((void*)0x0077EE45, DrawMeshSetBuffer_asm);

	// Hijacking a IDirect3DDevice8::SetTransform call in Direct3D_SetNearFarPlanes
	// to update the projection matrix.
	// This nops:
	// mov ecx, [eax] (device)
	// call dword ptr [ecx+94h] (device->SetTransform)
	WriteData((void*)0x00403234, 0x90i8, 8);
	WriteCall((void*)0x00403236, SetTransformHijack);
}

void releaseDepthTextures()
{
	for (auto& it : depthUnits)
	{
		it = nullptr;
	}

	for (auto& it : renderLayers)
	{
		it = nullptr;
	}

	depthSurface      = nullptr;
	depthBuffer       = nullptr;
	backBufferSurface = nullptr;
	backBuffer        = nullptr;
	origRenderTarget  = nullptr;
}

// These exports are for the window resize branch of the mod loader.
extern "C"
{
	EXPORT void __cdecl OnRenderDeviceLost()
	{
		for (auto& e : shaders)
		{
			if (e != nullptr)
			{
				e->OnLostDevice();
			}
		}

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

		UpdateParameterHandles();
	}

	EXPORT void __cdecl OnExit()
	{
		releaseDepthTextures();
		releaseParameters();
		releaseShaders();
	}
}
