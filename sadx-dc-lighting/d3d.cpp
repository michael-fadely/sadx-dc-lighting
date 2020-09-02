#include "stdafx.h"

#include <Windows.h>
#include <WinCrypt.h>

// Direct3D
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// MinHook
#include <MinHook.h>

// Standard library
#include <iomanip>
#include <sstream>
#include <vector>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "globals.h"
#include "../include/lanternapi.h"
#include "ShaderParameter.h"
#include "FileSystem.h"
#include "apiconfig.h"

// TODO: this file desperately needs to be split into multiple files

namespace param
{
	ShaderParameter<Texture>     PaletteA(1, nullptr, IShaderParameter::Type::vertex);
	ShaderParameter<Texture>     PaletteB(2, nullptr, IShaderParameter::Type::vertex);

	ShaderParameter<D3DXMATRIX>  WorldMatrix(0, {}, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXMATRIX>  wvMatrix(4, {}, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXMATRIX>  ProjectionMatrix(8, {}, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXMATRIX>  wvMatrixInvT(12, {}, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXMATRIX>  TextureTransform(16, {}, IShaderParameter::Type::vertex);

	ShaderParameter<D3DXVECTOR3> NormalScale(20, { 1.0f, 1.0f, 1.0f }, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXVECTOR3> LightDirection(21, { 0.0f, -1.0f, 0.0f }, IShaderParameter::Type::vertex);
	ShaderParameter<int>         DiffuseSource(22, 0, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXCOLOR>   MaterialDiffuse(23, { 0.0f, 0.0f, 0.0f, 0.0f }, IShaderParameter::Type::vertex);

	ShaderParameter<D3DXVECTOR4> Indices(24, { 0.0f, 0.0f, 0.0f, 0.0f }, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXVECTOR2> BlendFactor(25, { 0.0f, 0.0f }, IShaderParameter::Type::vertex);

	ShaderParameter<bool>        AllowVertexColor(26, true, IShaderParameter::Type::vertex);
	ShaderParameter<bool>        ForceDefaultDiffuse(27, false, IShaderParameter::Type::vertex);
	ShaderParameter<bool>        DiffuseOverride(28, false, IShaderParameter::Type::vertex);
	ShaderParameter<D3DXVECTOR3> DiffuseOverrideColor(29, { 1.0f, 1.0f, 1.0f }, IShaderParameter::Type::vertex);

	ShaderParameter<int>         FogMode(30, 0, IShaderParameter::Type::pixel);
	ShaderParameter<D3DXVECTOR3> FogConfig(31, {}, IShaderParameter::Type::pixel);
	ShaderParameter<D3DXCOLOR>   FogColor(32, {}, IShaderParameter::Type::pixel);
	ShaderParameter<float>       AlphaRef(33, 16.0f / 255.0f, IShaderParameter::Type::pixel);
	ShaderParameter<D3DXVECTOR3> ViewPosition(34, {}, IShaderParameter::Type::pixel);

	IShaderParameter* const parameters[] = {
		&PaletteA,
		&PaletteB,

		&WorldMatrix,
		&ProjectionMatrix,
		&wvMatrixInvT,
		&TextureTransform,

		&Indices,
		&BlendFactor,

		&NormalScale,
		&LightDirection,
		&DiffuseSource,
		&MaterialDiffuse,

		&AllowVertexColor,
		&ForceDefaultDiffuse,
		&DiffuseOverride,
		&DiffuseOverrideColor,

		&FogMode,
		&FogConfig,
		&FogColor,
		&AlphaRef,
		&ViewPosition,
	};

	static void release_parameters()
	{
		for (auto& i : parameters)
		{
			i->release();
		}
	}
}

namespace local
{
	static Trampoline* Direct3D_PerformLighting_t         = nullptr;
	static Trampoline* sub_77EAD0_t                       = nullptr;
	static Trampoline* sub_77EBA0_t                       = nullptr;
	static Trampoline* njCnkDrawModel_Chao_t              = nullptr;
	static Trampoline* ProcessPolyChunkList_t             = nullptr;
	static Trampoline* njDrawModel_SADX_t                 = nullptr;
	static Trampoline* njDrawModel_SADX_Dynamic_t         = nullptr;
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

	static decltype(DrawPrimitive_r)* DrawPrimitive_t                   = nullptr;
	static decltype(DrawIndexedPrimitive_r)* DrawIndexedPrimitive_t     = nullptr;
	static decltype(DrawPrimitiveUP_r)* DrawPrimitiveUP_t               = nullptr;
	static decltype(DrawIndexedPrimitiveUP_r)* DrawIndexedPrimitiveUP_t = nullptr;

	constexpr auto COMPILER_FLAGS = D3DXSHADER_PACKMATRIX_ROWMAJOR | D3DXSHADER_OPTIMIZATION_LEVEL3;

	constexpr auto DEFAULT_FLAGS = ShaderFlags_Alpha | ShaderFlags_Fog | ShaderFlags_Light | ShaderFlags_Texture;
	constexpr auto VS_MASK       = ShaderFlags_Texture | ShaderFlags_EnvMap | ShaderFlags_Light | ShaderFlags_Blend;
	constexpr auto PS_MASK       = ShaderFlags_Texture | ShaderFlags_Alpha | ShaderFlags_Fog | ShaderFlags_RangeFog;

	static Uint32 shader_flags = DEFAULT_FLAGS;
	static Uint32 last_flags   = DEFAULT_FLAGS;

	static D3DXVECTOR3 last_light_dir = {};

	static std::vector<uint8_t> shader_file;
	static std::unordered_map<ShaderFlags, VertexShader> vertex_shaders;
	static std::unordered_map<ShaderFlags, PixelShader> pixel_shaders;

	static bool   initialized   = false;
	static Uint32 drawing       = 0;
	static bool   using_shader  = false;
	static bool   supports_xrgb = false;

	static std::vector<D3DXMACRO> macros;

	DataPointer(Direct3DDevice8*, Direct3D_Device, 0x03D128B0);
	DataPointer(Direct3D8*, Direct3D_Object, 0x03D11F60);

	static auto sanitize(Uint32 flags)
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

		if (flags & ShaderFlags_RangeFog && !(flags & ShaderFlags_Fog))
		{
			flags &= ~ShaderFlags_RangeFog;
		}

		return flags;
	}

	static void free_shaders()
	{
		vertex_shaders.clear();
		pixel_shaders.clear();
		d3d::vertex_shader = nullptr;
		d3d::pixel_shader  = nullptr;
	}

	static void clear_shaders()
	{
		shader_file.clear();
		free_shaders();
	}

	static VertexShader get_vertex_shader(Uint32 flags);
	static PixelShader get_pixel_shader(Uint32 flags);

	static void create_shaders()
	{
		try
		{
			d3d::vertex_shader = get_vertex_shader(DEFAULT_FLAGS);
			d3d::pixel_shader  = get_pixel_shader(DEFAULT_FLAGS);

		#ifdef PRECOMPILE_SHADERS
			for (Uint32 i = 0; i < ShaderFlags_Count; i++)
			{
				auto flags = local::sanitize(i);

				auto vs = static_cast<ShaderFlags>(flags & VS_MASK);
				if (vertex_shaders.find(vs) == vertex_shaders.end())
				{
					get_vertex_shader(flags);
				}

				auto ps = static_cast<ShaderFlags>(flags & PS_MASK);
				if (pixel_shaders.find(ps) == pixel_shaders.end())
				{
					get_pixel_shader(flags);
				}
			}
		#endif

			for (auto& i : param::parameters)
			{
				i->commit_now(d3d::device);
			}
		}
		catch (std::exception& ex)
		{
			d3d::vertex_shader = nullptr;
			d3d::pixel_shader  = nullptr;
			MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
		}
	}

	static std::string to_string(Uint32 flags)
	{
		bool thing = false;
		std::stringstream result;

		while (flags != 0)
		{
			using namespace d3d;

			if (thing)
			{
				result << " | ";
			}

			if (flags & ShaderFlags_Fog)
			{
				flags &= ~ShaderFlags_Fog;
				result << "USE_FOG";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_RangeFog)
			{
				flags &= ~ShaderFlags_RangeFog;
				result << "RANGE_FOG";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_Blend)
			{
				flags &= ~ShaderFlags_Blend;
				result << "USE_BLEND";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_Light)
			{
				flags &= ~ShaderFlags_Light;
				result << "USE_LIGHT";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_Alpha)
			{
				flags &= ~ShaderFlags_Alpha;
				result << "USE_ALPHA";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_EnvMap)
			{
				flags &= ~ShaderFlags_EnvMap;
				result << "USE_ENVMAP";
				thing = true;
				continue;
			}

			if (flags & ShaderFlags_Texture)
			{
				flags &= ~ShaderFlags_Texture;
				result << "USE_TEXTURE";
				thing = true;
				continue;
			}

			break;
		}

		return result.str();
	}

	static void create_cache()
	{
		if (!filesystem::create_directory(globals::cache_path))
		{
			throw std::exception("Failed to create cache directory!");
		}
	}

	static void invalidate_cache()
	{
		if (filesystem::exists(globals::cache_path))
		{
			if (!filesystem::remove_all(globals::cache_path))
			{
				throw std::runtime_error("Failed to delete cache directory!");
			}
		}

		create_cache();
	}

	static auto shader_hash()
	{
		HCRYPTPROV hProv = 0;
		if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			throw std::runtime_error("CryptAcquireContext failed.");
		}

		HCRYPTHASH hHash = 0;
		if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
		{
			CryptReleaseContext(hProv, 0);
			throw std::runtime_error("CryptCreateHash failed.");
		}

		try
		{
			if (!CryptHashData(hHash, shader_file.data(), shader_file.size(), 0)
			    || !CryptHashData(hHash, reinterpret_cast<const BYTE*>(&COMPILER_FLAGS), sizeof(COMPILER_FLAGS), 0))
			{
				throw std::runtime_error("CryptHashData failed.");
			}

			// temporary
			DWORD buffer_size = sizeof(size_t);
			// actual size
			DWORD hash_size = 0;

			if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hash_size), &buffer_size, 0))
			{
				throw std::runtime_error("CryptGetHashParam failed while asking for hash buffer size.");
			}

			std::vector<uint8_t> result(hash_size);

			if (!CryptGetHashParam(hHash, HP_HASHVAL, result.data(), &hash_size, 0))
			{
				throw std::runtime_error("CryptGetHashParam failed while asking for hash value.");
			}

			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			return result;
		}
		catch (std::exception&)
		{
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			throw;
		}
	}

	static void load_shader_file(const std::basic_string<char>& shader_path)
	{
		std::ifstream file(shader_path, std::ios::ate);
		const auto size = file.tellg();
		file.seekg(0);

		if (file.is_open() && size > 0)
		{
			shader_file.resize(static_cast<size_t>(size));
			file.read(reinterpret_cast<char*>(shader_file.data()), size);
		}

		file.close();
	}

	static auto read_checksum(const std::basic_string<char>& checksum_path)
	{
		std::ifstream file(checksum_path, std::ios::ate | std::ios::binary);
		const auto size = file.tellg();
		file.seekg(0);

		if (size > 256 || size < 1)
		{
			throw std::runtime_error("checksum.bin file size out of range");
		}

		std::vector<uint8_t> data(static_cast<size_t>(size));
		file.read(reinterpret_cast<char*>(data.data()), data.size());
		file.close();

		return data;
	}

	static void store_checksum(const std::vector<uint8_t>& current_hash, const std::basic_string<char>& checksum_path)
	{
		invalidate_cache();

		std::ofstream file(checksum_path, std::ios::binary | std::ios::out);

		if (!file.is_open())
		{
			std::string error = "Failed to open file for writing: " + checksum_path;
			throw std::exception(error.c_str());
		}

		file.write(reinterpret_cast<const char*>(current_hash.data()), current_hash.size());
		file.close();
	}

	static auto shader_id(Uint32 flags)
	{
		std::stringstream result;

		result << std::hex
			<< std::setw(2)
			<< std::setfill('0')
			<< flags;

		return result.str();
	}

	static void populate_macros(Uint32 flags)
	{
		using namespace d3d;

		flags = sanitize(flags);

		if (flags & ShaderFlags_Texture)
		{
			macros.push_back({ "USE_TEXTURE", "1" });
		}

		if (flags & ShaderFlags_EnvMap)
		{
			macros.push_back({ "USE_ENVMAP", "1" });
		}

		if (flags & ShaderFlags_Light)
		{
			macros.push_back({ "USE_LIGHT", "1" });
		}

		if (flags & ShaderFlags_Blend)
		{
			macros.push_back({ "USE_BLEND", "1" });
		}

		if (flags & ShaderFlags_Alpha)
		{
			macros.push_back({ "USE_ALPHA", "1" });
		}

		if (flags & ShaderFlags_Fog)
		{
			macros.push_back({ "USE_FOG", "1" });
		}

		if (flags & ShaderFlags_RangeFog)
		{
			macros.push_back({ "RANGE_FOG", "1" });
		}

		macros.push_back({});
	}

	static __declspec(noreturn) void d3d_exception(const Buffer& buffer, HRESULT code)
	{
		using namespace std;

		stringstream message;

		message << '['
			<< hex
			<< setw(8)
			<< setfill('0')
			<< code;

		message << "] ";

		if (buffer != nullptr)
		{
			message << reinterpret_cast<const char*>(buffer->GetBufferPointer());
		}
		else
		{
			message << "Unspecified error.";
		}

		throw runtime_error(message.str());
	}

	static void check_shader_cache()
	{
		load_shader_file(globals::shader_path);

		const std::string checksum_path = filesystem::combine_path(globals::cache_path, "checksum.bin");
		const std::vector<uint8_t> current_hash = shader_hash();

		if (filesystem::exists(globals::cache_path))
		{
			if (!filesystem::exists(checksum_path))
			{
				store_checksum(current_hash, checksum_path);
			}
			else
			{
				const std::vector<uint8_t> last_hash(read_checksum(checksum_path));

				if (last_hash != current_hash)
				{
					store_checksum(current_hash, checksum_path);
				}
			}
		}
		else
		{
			store_checksum(current_hash, checksum_path);
		}
	}

	static void load_cached_shader(const std::string& sid_path, std::vector<uint8_t>& data)
	{
		std::ifstream file(sid_path, std::ios_base::ate | std::ios_base::binary);
		const auto size = file.tellg();
		file.seekg(0);

		if (size < 1)
		{
			throw std::runtime_error("corrupt vertex shader cache");
		}

		data.resize(static_cast<size_t>(size));
		file.read(reinterpret_cast<char*>(data.data()), data.size());
	}

	static void save_cached_shader(const std::string& sid_path, std::vector<uint8_t>& data)
	{
		std::ofstream file(sid_path, std::ios_base::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file for cache storage.");
		}

		file.write(reinterpret_cast<char*>(data.data()), data.size());
	}

	static VertexShader get_vertex_shader(Uint32 flags)
	{
		using namespace std;

		flags = sanitize(flags & VS_MASK);

		if (shader_file.empty())
		{
			check_shader_cache();
		}
		else
		{
			const auto it = vertex_shaders.find(static_cast<ShaderFlags>(flags));
			if (it != vertex_shaders.end())
			{
				return it->second;
			}
		}

		macros.clear();

		const string sid_path = filesystem::combine_path(globals::cache_path, shader_id(flags) + ".vs");
		bool is_cached = filesystem::exists(sid_path);

		vector<uint8_t> data;

		if (is_cached)
		{
			PrintDebug("[lantern] Loading cached vertex shader #%02d: %08X (%s)\n",
			           vertex_shaders.size(), flags, to_string(flags).c_str());

			load_cached_shader(sid_path, data);
		}
		else
		{
			PrintDebug("[lantern] Compiling vertex shader #%02d: %08X (%s)\n",
			           vertex_shaders.size(), flags, to_string(flags).c_str());

			populate_macros(flags);

			Buffer errors;
			Buffer buffer;

			auto result = D3DXCompileShader(reinterpret_cast<char*>(shader_file.data()), shader_file.size(), macros.data(), nullptr,
			                                "vs_main", "vs_3_0", COMPILER_FLAGS, &buffer, &errors, nullptr);

			if (FAILED(result) || errors != nullptr)
			{
				d3d_exception(errors, result);
			}

			data.resize(static_cast<size_t>(buffer->GetBufferSize()));
			memcpy(data.data(), buffer->GetBufferPointer(), data.size());
		}

		VertexShader shader;
		auto result = d3d::device->CreateVertexShader(reinterpret_cast<const DWORD*>(data.data()), &shader);

		if (FAILED(result))
		{
			d3d_exception(nullptr, result);
		}

		if (!is_cached)
		{
			save_cached_shader(sid_path, data);
		}

		vertex_shaders[static_cast<ShaderFlags>(flags)] = shader;
		return shader;
	}

	static PixelShader get_pixel_shader(Uint32 flags)
	{
		using namespace std;

		if (shader_file.empty())
		{
			check_shader_cache();
		}
		else
		{
			const auto it = pixel_shaders.find(static_cast<ShaderFlags>(flags & PS_MASK));
			if (it != pixel_shaders.end())
			{
				return it->second;
			}
		}

		macros.clear();

		flags = sanitize(flags & PS_MASK);

		const string sid_path = filesystem::combine_path(globals::cache_path, shader_id(flags) + ".ps");
		bool is_cached = filesystem::exists(sid_path);

		vector<uint8_t> data;

		if (is_cached)
		{
			PrintDebug("[lantern] Loading cached pixel shader #%02d: %08X (%s)\n",
			           pixel_shaders.size(), flags, to_string(flags).c_str());

			load_cached_shader(sid_path, data);
		}
		else
		{
			PrintDebug("[lantern] Compiling pixel shader #%02d: %08X (%s)\n",
			           pixel_shaders.size(), flags, to_string(flags).c_str());

			populate_macros(flags);

			Buffer errors;
			Buffer buffer;

			auto result = D3DXCompileShader(reinterpret_cast<char*>(shader_file.data()), shader_file.size(), macros.data(), nullptr,
			                                "ps_main", "ps_3_0", COMPILER_FLAGS, &buffer, &errors, nullptr);

			if (FAILED(result) || errors != nullptr)
			{
				d3d_exception(errors, result);
			}

			data.resize(static_cast<size_t>(buffer->GetBufferSize()));
			memcpy(data.data(), buffer->GetBufferPointer(), data.size());
		}

		PixelShader shader;
		auto result = d3d::device->CreatePixelShader(reinterpret_cast<const DWORD*>(data.data()), &shader);

		if (FAILED(result))
		{
			d3d_exception(nullptr, result);
		}

		if (!is_cached)
		{
			save_cached_shader(sid_path, data);
		}

		pixel_shaders[static_cast<ShaderFlags>(flags & PS_MASK)] = shader;
		return shader;
	}

	static void set_light_parameters()
	{
		if (apiconfig::override_light_dir)
		{
			param::LightDirection = *reinterpret_cast<const D3DXVECTOR3*>(&apiconfig::light_dir_override);
			return;
		}

		D3DLIGHT9 light;
		d3d::device->GetLight(0, &light);

		const auto dir = -*reinterpret_cast<const D3DXVECTOR3*>(&light.Direction);
		param::LightDirection = dir;
	}

	static void begin()
	{
		++drawing;
	}

	static void end()
	{
		if (drawing > 0 && --drawing < 1)
		{
			drawing = 0;
			d3d::do_effect = false;
			d3d::reset_overrides();
		}
	}

	static void shader_end()
	{
		if (using_shader)
		{
			d3d::device->SetPixelShader(nullptr);
			d3d::device->SetVertexShader(nullptr);
			using_shader = false;
		}
	}

	static void shader_start()
	{
		if (!d3d::do_effect || !drawing)
		{
			shader_end();
			return;
		}

		set_light_parameters();
		globals::palettes.apply_parameters();

		bool changes = false;

		const auto flags = sanitize(shader_flags);

		if (flags != last_flags)
		{
			VertexShader vs;
			PixelShader ps;

			changes = true;
			last_flags = flags;

			try
			{
				vs = get_vertex_shader(flags);
				ps = get_pixel_shader(flags);
			}
			catch (std::exception& ex)
			{
				shader_end();
				MessageBoxA(WindowHandle, ex.what(), "Shader creation failed", MB_OK | MB_ICONERROR);
				return;
			}

			if (!using_shader || vs != d3d::vertex_shader)
			{
				d3d::vertex_shader = vs;
				d3d::device->SetVertexShader(d3d::vertex_shader);
			}

			if (!using_shader || ps != d3d::pixel_shader)
			{
				d3d::pixel_shader = ps;
				d3d::device->SetPixelShader(d3d::pixel_shader);
			}
		}
		else if (!using_shader)
		{
			d3d::device->SetVertexShader(d3d::vertex_shader);
			d3d::device->SetPixelShader(d3d::pixel_shader);
		}

		if (changes || !IShaderParameter::values_assigned.empty())
		{
			for (auto& it : IShaderParameter::values_assigned)
			{
				it->commit(d3d::device);
			}

			IShaderParameter::values_assigned.clear();
		}

		using_shader = true;
	}

#define MHOOK(NAME) MH_CreateHook(vtbl[IndexOf_ ## NAME], NAME ## _r, (LPVOID*)&NAME ## _t)

	static void hook_vtable()
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

		MHOOK(DrawPrimitive);
		MHOOK(DrawIndexedPrimitive);
		MHOOK(DrawPrimitiveUP);
		MHOOK(DrawIndexedPrimitiveUP);

		MH_EnableHook(MH_ALL_HOOKS);
	}

#pragma region Trampolines

	template <typename T, typename... Args>
	static void run_trampoline(const T& original, Args ... args)
	{
		begin();
		original(args...);
		end();
	}

	static void __cdecl sub_77EAD0_r(void* a1, int a2, int a3)
	{
		begin();
		run_trampoline(TARGET_DYNAMIC(sub_77EAD0), a1, a2, a3);
		end();
	}

	static void __cdecl sub_77EBA0_r(void* a1, int a2, int a3)
	{
		begin();
		run_trampoline(TARGET_DYNAMIC(sub_77EBA0), a1, a2, a3);
		end();
	}

	static Sint32 __cdecl njCnkDrawModel_Chao_r(NJS_CNK_MODEL *model)
	{
		d3d::do_effect = true;
		begin();

		const auto original = reinterpret_cast<decltype(njCnkDrawModel_Chao_r)*>(njCnkDrawModel_Chao_t->Target());
		const auto result = original(model);

		end();
		return result;
	}

	// Dummy interface which has the same number of function pointers as [I]Direct3DDevice8.
	// Used for handling specific draw calls used for chunk model rendering.
	struct chunk_d3d8_t
	{
		void* ptr;
		void* ptrs[97] {};

		static uint32_t flags;

		chunk_d3d8_t()
		{
			ptr = &ptrs[0];

			ptrs[50] = &chunk_d3d8_t::SetRenderState;
			ptrs[63] = &chunk_d3d8_t::SetTextureStageState;
			ptrs[76] = &chunk_d3d8_t::SetVertexShader;
		}

		static void set_palettes()
		{
			uint32_t temp_flags = flags;

			if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
			{
				temp_flags = _nj_constant_attr_or_ | (_nj_constant_attr_and_ & temp_flags);
			}

			globals::palettes.set_palettes(2, temp_flags);
		}

		static HRESULT __stdcall SetRenderState(Direct3DDevice8* /*This*/, D3DRENDERSTATETYPE State, DWORD Value)
		{
			switch (State)
			{
				case D3DRS_SPECULARENABLE:
					if (Value != 0)
					{
						flags &= ~NJD_FLAG_IGNORE_SPECULAR;
					}
					else
					{
						flags |= NJD_FLAG_IGNORE_SPECULAR;
					}
					break;
				case D3DRS_LIGHTING:
					if (Value != 0)
					{
						flags &= ~NJD_FLAG_IGNORE_LIGHT;
					}
					else
					{
						flags |= NJD_FLAG_IGNORE_LIGHT;
					}
					break;
				case D3DRS_ALPHABLENDENABLE:
					if (Value != 0)
					{
						flags |= NJD_FLAG_USE_ALPHA;
					}
					else
					{
						flags &= ~NJD_FLAG_USE_ALPHA;
					}
					break;

				default:
					break;
			}

			set_palettes();

			d3d::set_flags(ShaderFlags_Alpha,   (flags & NJD_FLAG_USE_ALPHA)    != 0);
			d3d::set_flags(ShaderFlags_EnvMap,  (flags & NJD_FLAG_USE_ENV)      != 0);
			d3d::set_flags(ShaderFlags_Light,   (flags & NJD_FLAG_IGNORE_LIGHT) == 0);

			return Direct3D_Device->SetRenderState(State, Value);
		}

		static HRESULT __stdcall SetTextureStageState(Direct3DDevice8* /*This*/, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
		{
			if (Type == D3DTSS_TEXCOORDINDEX)
			{
				if (Value != 0)
				{
					flags |= NJD_FLAG_USE_ENV;
				}
				else
				{
					flags &= ~NJD_FLAG_USE_ENV;
				}

				d3d::set_flags(ShaderFlags_EnvMap, (flags & NJD_FLAG_USE_ENV) != 0);
				set_palettes();
			}

			return Direct3D_Device->SetTextureStageState(Stage, Type, Value);
		}

		static HRESULT __stdcall SetVertexShader(Direct3DDevice8* /*This*/, DWORD Handle)
		{
			return Direct3D_Device->SetVertexShader(Handle);
		}
	};

	static chunk_d3d8_t chunk_d3d8 {};
	static chunk_d3d8_t* chunk_d3d8_ptr = &chunk_d3d8;
	uint32_t chunk_d3d8_t::flags = 0;

	static void __cdecl material_setup_hijack(int i, int16_t* p)
	{
		auto fn = reinterpret_cast<void(**)(int16_t*)>(0x038A5E1C + (4 * i));
		(*fn)(p);

		D3DMATERIAL8 material {};
		Direct3D_Device->GetMaterial(&material);

		param::MaterialDiffuse = material.Diffuse;
	}

	static const auto material_setup_hijack_return = reinterpret_cast<void*>(0x0078ED4D);

	void __declspec(naked) material_setup_hijack_asm()
	{
		__asm
		{
			push edx
			call material_setup_hijack
			add esp, 4
			jmp material_setup_hijack_return
		}
	}

	bool poly_chunk_list_called = false;

	// TODO: fix material diffuse color
	static void __fastcall ProcessPolyChunkList_r(Sint16 *this_)
	{
		const bool first_caller = !poly_chunk_list_called;
		poly_chunk_list_called = true;

		const Uint32 backup_flags = shader_flags;
		const bool backup_allow_vcolor = param::AllowVertexColor.value();
		const int backup_diffuse_source = param::DiffuseSource.value();

		if (first_caller)
		{
			chunk_d3d8_t::flags = NJD_FLAG_IGNORE_SPECULAR;

			d3d::do_effect = true;

			shader_flags &= ShaderFlags_Fog | ShaderFlags_RangeFog;

			param::AllowVertexColor = true;
			param::DiffuseSource = D3DMCS_COLOR1;
		}
		
		run_trampoline(TARGET_DYNAMIC(ProcessPolyChunkList), this_);

		if (first_caller)
		{
			chunk_d3d8_t::flags = 0;

			d3d::do_effect = false;
			shader_flags = backup_flags;

			param::DiffuseSource = backup_diffuse_source;
			param::AllowVertexColor = backup_allow_vcolor;

			poly_chunk_list_called = false;
		}
	}

	void __fastcall ChunkTextureFlip_r(__int16 a1)
	{
		chunk_d3d8_t::flags |= NJD_FLAG_USE_TEXTURE;
		d3d::set_flags(ShaderFlags_Texture, true);
		chunk_d3d8_t::set_palettes();

		ChunkTextureFlip(a1);
	}

	static void __cdecl njDrawModel_SADX_r(NJS_MODEL_SADX* a1)
	{
		begin();

		if (a1 && a1->nbMat && a1->mats)
		{
			globals::first_material = true;

			const auto _control_3d = _nj_control_3d_flag_;
			const auto _attr_or    = _nj_constant_attr_or_;
			const auto _attr_and   = _nj_constant_attr_and_;

			run_trampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);

			_nj_control_3d_flag_   = _control_3d;
			_nj_constant_attr_and_ = _attr_and;
			_nj_constant_attr_or_  = _attr_or;
		}
		else
		{
			run_trampoline(TARGET_DYNAMIC(njDrawModel_SADX), a1);
		}

		end();
	}

	static void __cdecl njDrawModel_SADX_Dynamic_r(NJS_MODEL_SADX* a1)
	{
		begin();

		if (a1 && a1->nbMat && a1->mats)
		{
			globals::first_material = true;

			const auto _control_3d = _nj_control_3d_flag_;
			const auto _attr_or    = _nj_constant_attr_or_;
			const auto _attr_and   = _nj_constant_attr_and_;

			run_trampoline(TARGET_DYNAMIC(njDrawModel_SADX_Dynamic), a1);

			_nj_control_3d_flag_   = _control_3d;
			_nj_constant_attr_and_ = _attr_and;
			_nj_constant_attr_or_  = _attr_or;
		}
		else
		{
			run_trampoline(TARGET_DYNAMIC(njDrawModel_SADX_Dynamic), a1);
		}

		end();
	}

	static void __fastcall PolyBuff_DrawTriangleStrip_r(PolyBuff* _this)
	{
		begin();
		run_trampoline(TARGET_DYNAMIC(PolyBuff_DrawTriangleStrip), _this);
		end();
	}

	static void __fastcall PolyBuff_DrawTriangleList_r(PolyBuff* _this)
	{
		begin();
		run_trampoline(TARGET_DYNAMIC(PolyBuff_DrawTriangleList), _this);
		end();
	}

	static void check_format()
	{
		const auto fmt = *reinterpret_cast<D3DFORMAT*>(reinterpret_cast<char*>(0x03D0FDC0) + 0x08);

		auto result = Direct3D_Object->CheckDeviceFormat(DisplayAdapter, D3DDEVTYPE_HAL, fmt,
		                                                 D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);

		if (result == D3D_OK)
		{
			supports_xrgb = true;
			return;
		}

		result = Direct3D_Object->CheckDeviceFormat(DisplayAdapter, D3DDEVTYPE_HAL, fmt,
		                                            D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F);

		if (result != D3D_OK)
		{
			MessageBoxA(WindowHandle, "Your GPU does not support any (reasonable) vertex texture sample formats.",
			            "Insufficient GPU support", MB_OK | MB_ICONERROR);

			Exit();
		}
	}

	// ReSharper disable once CppDeclaratorNeverUsed
	static void __cdecl CreateDirect3DDevice_c(int behavior, int type)
	{
		if (Direct3D_Device == nullptr && Direct3D_Object != nullptr)
		{
			check_format();
		}

		auto orig = CreateDirect3DDevice_t->Target();
		auto _type = type;

		(void)orig;
		(void)_type;
		(void)behavior;

		__asm
		{
			push _type
			mov edx, behavior
			call orig
		}

		if (Direct3D_Device != nullptr && !initialized)
		{
			d3d::device = Direct3D_Device->GetProxyInterface();

			initialized = true;
			d3d::load_shader();
			hook_vtable();
		}
	}

	static void __declspec(naked) CreateDirect3DDevice_r()
	{
		__asm
		{
			push [esp + 04h] // type
			push edx // behavior

			call CreateDirect3DDevice_c

			pop edx // behavior
			add esp, 4
			retn 4
		}
	}

	static void __cdecl Direct3D_SetWorldTransform_r()
	{
		TARGET_DYNAMIC(Direct3D_SetWorldTransform)();

		if (!LanternInstance::use_palette())
		{
			return;
		}

		param::WorldMatrix = WorldMatrix;

		auto wvMatrix = D3DXMATRIX(WorldMatrix) * D3DXMATRIX(ViewMatrix);
		param::wvMatrix = wvMatrix;

		D3DXMatrixInverse(&wvMatrix, nullptr, &wvMatrix);
		D3DXMatrixTranspose(&wvMatrix, &wvMatrix);
		// The inverse transpose matrix is used for environment mapping.
		param::wvMatrixInvT = wvMatrix;
	}

	static void __stdcall Direct3D_SetProjectionMatrix_r(float hfov, float nearPlane, float farPlane)
	{
		TARGET_DYNAMIC(Direct3D_SetProjectionMatrix)(hfov, nearPlane, farPlane);

		// The view matrix can also be set here if necessary.
		param::ProjectionMatrix = D3DXMATRIX(ProjectionMatrix) * D3DXMATRIX(TransformationMatrix);
		param::ViewPosition = D3DXVECTOR3(InverseViewMatrix._41, InverseViewMatrix._42, InverseViewMatrix._43);
	}

	static void __cdecl Direct3D_SetViewportAndTransform_r()
	{
		const auto original = TARGET_DYNAMIC(Direct3D_SetViewportAndTransform);

		bool invalid = TransformAndViewportInvalid != 0;
		original();

		if (invalid)
		{
			param::ProjectionMatrix = D3DXMATRIX(ProjectionMatrix) * D3DXMATRIX(TransformationMatrix);
		}
	}

	static void __cdecl Direct3D_PerformLighting_r(int type)
	{
		const auto target = TARGET_DYNAMIC(Direct3D_PerformLighting);

		if (!LanternInstance::use_palette())
		{
			globals::light_type = 0;
			target(type);
			return;
		}

		// This specifically forces light type 0 to prevent
		// the light direction from being overwritten.
		target(0);

		const auto div2 = type / 2;

		if (div2 != CurrentLightType)
		{
			CurrentLightType = div2;

			// deliberately avoiding the call to SetCurrentLightType_Copy
			// to maintain onion-blur compatibility
			CurrentLightType_Copy = div2;
		}

		d3d::set_flags(ShaderFlags_Light, true);
		globals::palettes.set_palettes(type, 0);
		set_light_parameters();
	}


#define D3D_ORIG(NAME) \
	NAME ## _t

	static HRESULT __stdcall DrawPrimitive_r(IDirect3DDevice9* _this,
	                                         D3DPRIMITIVETYPE PrimitiveType,
	                                         UINT StartVertex,
	                                         UINT PrimitiveCount)
	{
		shader_start();
		auto result = D3D_ORIG(DrawPrimitive)(_this, PrimitiveType, StartVertex, PrimitiveCount);
		shader_end();
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
		shader_start();
		auto result = D3D_ORIG(DrawIndexedPrimitive)(_this, PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		shader_end();
		return result;
	}

	static HRESULT __stdcall DrawPrimitiveUP_r(IDirect3DDevice9* _this,
	                                           D3DPRIMITIVETYPE PrimitiveType,
	                                           UINT PrimitiveCount,
	                                           CONST void* pVertexStreamZeroData,
	                                           UINT VertexStreamZeroStride)
	{
		shader_start();
		auto result = D3D_ORIG(DrawPrimitiveUP)(_this, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
		shader_end();
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
		shader_start();
		auto result = D3D_ORIG(DrawIndexedPrimitiveUP)(_this, PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
		shader_end();
		return result;
	}

	// ReSharper disable once CppDeclaratorNeverUsed
	static void __stdcall DrawMeshSetBuffer_c(MeshSetBuffer* buffer)
	{
		if (!buffer->FVF)
		{
			return;
		}

		Direct3D_Device->SetVertexShader(buffer->FVF);
		Direct3D_Device->SetStreamSource(0, buffer->VertexBuffer, buffer->Size);

		const auto index_buffer = buffer->IndexBuffer;
		if (index_buffer)
		{
			Direct3D_Device->SetIndices(index_buffer, 0);

			begin();

			Direct3D_Device->DrawIndexedPrimitive(buffer->PrimitiveType,
			                                      buffer->MinIndex,
			                                      buffer->NumVertecies,
			                                      buffer->StartIndex,
			                                      buffer->PrimitiveCount);
		}
		else
		{
			begin();

			Direct3D_Device->DrawPrimitive(buffer->PrimitiveType,
			                               buffer->StartIndex,
			                               buffer->PrimitiveCount);
		}

		end();
	}

	// ReSharper disable once CppDeclaratorNeverUsed
	static const auto loc_77EF09 = reinterpret_cast<void*>(0x0077EF09);

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
		param::ProjectionMatrix = *matrix;
		return _device->SetTransform(type, matrix);
	}
#pragma endregion
}

namespace d3d
{
	IDirect3DDevice9* device = nullptr;
	VertexShader vertex_shader;
	PixelShader pixel_shader;
	bool do_effect = false;

	bool supports_xrgb()
	{
		return local::supports_xrgb;
	}

	void reset_overrides()
	{
		if (LanternInstance::diffuse_override_is_temp)
		{
			LanternInstance::diffuse_override = false;
			param::DiffuseOverride = false;
		}

		if (LanternInstance::specular_override_is_temp)
		{
			LanternInstance::specular_override = false;
		}

		if (apiconfig::override_light_dir)
		{
			param::LightDirection = local::last_light_dir;
			apiconfig::override_light_dir = false;
		}

		if (apiconfig::alpha_ref_is_temp)
		{
			param::AlphaRef = apiconfig::alpha_ref_value;
			apiconfig::alpha_ref_is_temp = false;
		}

		param::ForceDefaultDiffuse = false;
	}

	void load_shader()
	{
		if (!local::initialized)
		{
			return;
		}

		local::clear_shaders();
		local::create_shaders();
	}

	void set_flags(Uint32 flags, bool add)
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

	bool shaders_null()
	{
		return vertex_shader == nullptr || pixel_shader == nullptr;
	}

	void init_trampolines()
	{
		using namespace local;

		Direct3D_PerformLighting_t         = new Trampoline(0x00412420, 0x00412426, Direct3D_PerformLighting_r);
		sub_77EAD0_t                       = new Trampoline(0x0077EAD0, 0x0077EAD7, sub_77EAD0_r);
		sub_77EBA0_t                       = new Trampoline(0x0077EBA0, 0x0077EBA5, sub_77EBA0_r);
		njCnkDrawModel_Chao_t              = new Trampoline(0x0078AA10, 0x0078AA15, njCnkDrawModel_Chao_r);
		ProcessPolyChunkList_t             = new Trampoline(0x0078EB50, 0x0078EB58, ProcessPolyChunkList_r);
		njDrawModel_SADX_t                 = new Trampoline(0x0077EDA0, 0x0077EDAA, njDrawModel_SADX_r);
		njDrawModel_SADX_Dynamic_t         = new Trampoline(0x00784AE0, 0x00784AE5, njDrawModel_SADX_Dynamic_r);
		Direct3D_SetProjectionMatrix_t     = new Trampoline(0x00791170, 0x00791175, Direct3D_SetProjectionMatrix_r);
		Direct3D_SetViewportAndTransform_t = new Trampoline(0x007912E0, 0x007912E8, Direct3D_SetViewportAndTransform_r);
		Direct3D_SetWorldTransform_t       = new Trampoline(0x00791AB0, 0x00791AB5, Direct3D_SetWorldTransform_r);
		CreateDirect3DDevice_t             = new Trampoline(0x00794000, 0x00794007, CreateDirect3DDevice_r);
		PolyBuff_DrawTriangleStrip_t       = new Trampoline(0x00794760, 0x00794767, PolyBuff_DrawTriangleStrip_r);
		PolyBuff_DrawTriangleList_t        = new Trampoline(0x007947B0, 0x007947B7, PolyBuff_DrawTriangleList_r);

		// Hijack all the D3D member functions used in ProcessPolyChunkList (0x0078EB50).
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EB64 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EC8B + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078ECAE + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078ECFE + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078ED21 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078ED51 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078ED94 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EDCB + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EDE7 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EE0F + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EE26 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EE3A + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EE5B + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EE6F + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EEAB + 2), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078EECD + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078BB59 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078BB89 + 1), &chunk_d3d8_ptr);

		// Hijack blending mode adjustments in a function used exclusively by ProcessPolyChunkList.
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078DB90 + 1), &chunk_d3d8_ptr);
		WriteData(reinterpret_cast<chunk_d3d8_t***>(0x0078DBB4 + 1), &chunk_d3d8_ptr);

		// Ensure diffuse material color is submitted to the shader. The mod loader replaces a
		// function and adds a parameter in order to fix specular lighting on chunk models.
		// This doesn't leave room for a convenient trampoline, so assembly it is.
		WriteJump(reinterpret_cast<void*>(0x0078ED46), material_setup_hijack_asm);

		// Used to enable textures for the next-rendered chunk model. This is replacing a call
		// from within ProcessPolyChunkList.
		WriteCall(reinterpret_cast<void*>(0x0078ECEA), ChunkTextureFlip_r);

		WriteJump(reinterpret_cast<void*>(0x0077EE45), DrawMeshSetBuffer_asm);

		// Hijacking a IDirect3DDevice8::SetTransform call in Direct3D_SetNearFarPlanes
		// to update the projection matrix.
		// This nops:
		// mov ecx, [eax] (device)
		// call dword ptr [ecx+94h] (device->SetTransform)
		WriteData<8>(reinterpret_cast<void*>(0x00403234), 0x90i8);
		WriteCall(reinterpret_cast<void*>(0x00403236), SetTransformHijack);
	}
}

extern "C"
{
	using namespace local;

	EXPORT void __cdecl OnRenderDeviceLost()
	{
		end();
		free_shaders();
	}

	EXPORT void __cdecl OnRenderDeviceReset()
	{
		create_shaders();
	}

	EXPORT void __cdecl OnExit()
	{
		param::release_parameters();
		free_shaders();
	}
}
