#include "stdafx.h"

#include <Windows.h>
#include <WinCrypt.h>

// d3d8to9
#include <d3d8to9.hpp>
#include <d3dcompiler.h>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Standard library
#include <iomanip>
#include <sstream>
#include <vector>

// Local
#include "d3d.h"
#include "globals.h"
#include "../include/lanternapi.h"
#include "FileSystem.h"
#include "apiconfig.h"
#include "stupidbullshit.h"
#include "PaletteParameters.h"
#include "LanternParameters.h"

namespace param
{
	//ShaderParameter<Texture>     PaletteA(1, nullptr, IShaderParameter::Type::vertex);
	//ShaderParameter<Texture>     PaletteB(2, nullptr, IShaderParameter::Type::vertex);

	Texture PaletteA, PaletteB;

	PaletteParameters palette {};
	LanternParameters lantern {};

	ComPtr<ID3D11Buffer> palette_cbuffer;
	ComPtr<ID3D11Buffer> lantern_cbuffer;

	//ShaderParameter<float3> ViewPosition(34, {}, IShaderParameter::Type::pixel);
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

	constexpr auto COMPILER_FLAGS = 0; // HACK

	constexpr auto DEFAULT_FLAGS = LanternShaderFlags_Alpha | LanternShaderFlags_Fog | LanternShaderFlags_Light | LanternShaderFlags_Texture;
	constexpr auto VS_MASK       = LanternShaderFlags_Texture | LanternShaderFlags_EnvMap | LanternShaderFlags_Light | LanternShaderFlags_Blend;
	constexpr auto PS_MASK       = LanternShaderFlags_Texture | LanternShaderFlags_Alpha | LanternShaderFlags_Fog | LanternShaderFlags_RangeFog;

	static Uint32 shader_flags = DEFAULT_FLAGS;
	static Uint32 last_flags   = DEFAULT_FLAGS;

	static float3 last_light_dir = {};

	static std::vector<uint8_t> shader_file;
	static std::unordered_map<LanternShaderFlags, VertexShader> vertex_shaders;
	static std::unordered_map<LanternShaderFlags, PixelShader> pixel_shaders;

	static bool   initialized   = false;
	static Uint32 drawing       = 0;
	static bool   using_shader  = false;
	static bool   supports_xrgb = false;

	static std::vector<D3D_SHADER_MACRO> macros;

	static auto sanitize(Uint32 flags)
	{
		flags &= LanternShaderFlags_Mask;

		if (flags & LanternShaderFlags_Blend && !(flags & LanternShaderFlags_Light))
		{
			flags &= ~LanternShaderFlags_Blend;
		}

		if (flags & LanternShaderFlags_EnvMap && !(flags & LanternShaderFlags_Texture))
		{
			flags &= ~LanternShaderFlags_EnvMap;
		}

		if (flags & LanternShaderFlags_RangeFog && !(flags & LanternShaderFlags_Fog))
		{
			flags &= ~LanternShaderFlags_RangeFog;
		}

		return flags;
	}

	static void free_shaders()
	{
		vertex_shaders.clear();
		pixel_shaders.clear();
		d3d::vertex_shader = {};
		d3d::pixel_shader  = {};
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
			for (Uint32 i = 0; i < LanternShaderFlags_Count; i++)
			{
				auto flags = local::sanitize(i);

				auto vs = static_cast<LanternShaderFlags>(flags & VS_MASK);
				if (vertex_shaders.find(vs) == vertex_shaders.end())
				{
					get_vertex_shader(flags);
				}

				auto ps = static_cast<LanternShaderFlags>(flags & PS_MASK);
				if (pixel_shaders.find(ps) == pixel_shaders.end())
				{
					get_pixel_shader(flags);
				}
			}
		#endif
		}
		catch (std::exception& ex)
		{
			d3d::vertex_shader = {};
			d3d::pixel_shader  = {};
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

			if (flags & LanternShaderFlags_Fog)
			{
				flags &= ~LanternShaderFlags_Fog;
				result << "USE_FOG";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_RangeFog)
			{
				flags &= ~LanternShaderFlags_RangeFog;
				result << "RANGE_FOG";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_Blend)
			{
				flags &= ~LanternShaderFlags_Blend;
				result << "USE_BLEND";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_Light)
			{
				flags &= ~LanternShaderFlags_Light;
				result << "USE_LIGHT";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_Alpha)
			{
				flags &= ~LanternShaderFlags_Alpha;
				result << "USE_ALPHA";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_EnvMap)
			{
				flags &= ~LanternShaderFlags_EnvMap;
				result << "USE_ENVMAP";
				thing = true;
				continue;
			}

			if (flags & LanternShaderFlags_Texture)
			{
				flags &= ~LanternShaderFlags_Texture;
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

		if (flags & LanternShaderFlags_Texture)
		{
			macros.push_back({ "USE_TEXTURE", "1" });
		}

		if (flags & LanternShaderFlags_EnvMap)
		{
			macros.push_back({ "USE_ENVMAP", "1" });
		}

		if (flags & LanternShaderFlags_Light)
		{
			macros.push_back({ "USE_LIGHT", "1" });
		}

		if (flags & LanternShaderFlags_Blend)
		{
			macros.push_back({ "USE_BLEND", "1" });
		}

		if (flags & LanternShaderFlags_Alpha)
		{
			macros.push_back({ "USE_ALPHA", "1" });
		}

		if (flags & LanternShaderFlags_Fog)
		{
			macros.push_back({ "USE_FOG", "1" });
		}

		if (flags & LanternShaderFlags_RangeFog)
		{
			macros.push_back({ "RANGE_FOG", "1" });
		}

		macros.push_back({});
	}

	static __declspec(noreturn) void d3d_exception(const ComPtr<ID3DBlob>& buffer, HRESULT code)
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
			const auto it = vertex_shaders.find(static_cast<LanternShaderFlags>(flags));
			if (it != vertex_shaders.end())
			{
				return it->second;
			}
		}

		macros.clear();

		const string sid_path = ::filesystem::combine_path(globals::cache_path, shader_id(flags) + ".vs");
		bool is_cached = ::filesystem::exists(sid_path);

		vector<uint8_t> data;
		VertexShader shader;

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

			ComPtr<ID3DBlob> errors;

			auto result = D3DCompile(shader_file.data(), shader_file.size(), sid_path.c_str(), macros.data(),
			                         nullptr, "vs_main", "vs_4_0", 0, 0, &shader.blob, &errors);

			if (FAILED(result) || errors != nullptr)
			{
				d3d_exception(errors, result);
			}

			data.resize(static_cast<size_t>(shader.blob->GetBufferSize()));
			memcpy(data.data(), shader.blob->GetBufferPointer(), data.size());
		}

		auto result = Direct3D_Device->device->CreateVertexShader(data.data(), data.size(), nullptr, &shader.shader);

		if (FAILED(result))
		{
			d3d_exception(nullptr, result);
		}

		if (!is_cached)
		{
			save_cached_shader(sid_path, data);
		}

		vertex_shaders[static_cast<LanternShaderFlags>(flags)] = shader;
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
			const auto it = pixel_shaders.find(static_cast<LanternShaderFlags>(flags & PS_MASK));
			if (it != pixel_shaders.end())
			{
				return it->second;
			}
		}

		macros.clear();

		flags = sanitize(flags & PS_MASK);

		const string sid_path = ::filesystem::combine_path(globals::cache_path, shader_id(flags) + ".ps");
		bool is_cached = ::filesystem::exists(sid_path);

		vector<uint8_t> data;
		PixelShader shader;

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

			ComPtr<ID3DBlob> errors;

			auto result = D3DCompile(shader_file.data(), shader_file.size(), sid_path.c_str(), macros.data(),
			                         nullptr, "ps_main", "ps_4_0", 0, 0, &shader.blob, &errors);

			if (FAILED(result) || errors != nullptr)
			{
				d3d_exception(errors, result);
			}

			data.resize(static_cast<size_t>(shader.blob->GetBufferSize()));
			memcpy(data.data(), shader.blob->GetBufferPointer(), data.size());
		}

		auto result = Direct3D_Device->device->CreatePixelShader(data.data(), data.size(), nullptr, &shader.shader);

		if (FAILED(result))
		{
			d3d_exception(nullptr, result);
		}

		if (!is_cached)
		{
			save_cached_shader(sid_path, data);
		}

		pixel_shaders[static_cast<LanternShaderFlags>(flags & PS_MASK)] = shader;
		return shader;
	}

	static void set_light_parameters()
	{
		if (apiconfig::override_light_dir)
		{
			param::lantern.light_direction = *reinterpret_cast<const float3*>(&apiconfig::light_dir_override);
			return;
		}

		D3DLIGHT8 light;
		Direct3D_Device->GetLight(0, &light);

		const auto dir = -*reinterpret_cast<const float3*>(&light.Direction);
		param::lantern.light_direction = dir;
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
			// TODO: d3d::device->SetPixelShader(nullptr);
			// TODO: d3d::device->SetVertexShader(nullptr);
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
				Direct3D_Device->context->VSSetShader(d3d::vertex_shader.shader.Get(), nullptr, 0);
			}

			if (!using_shader || ps != d3d::pixel_shader)
			{
				d3d::pixel_shader = ps;
				Direct3D_Device->context->PSSetShader(d3d::pixel_shader.shader.Get(), nullptr, 0);
			}
		}
		else if (!using_shader)
		{
			Direct3D_Device->context->VSSetShader(d3d::vertex_shader.shader.Get(), nullptr, 0);
			Direct3D_Device->context->PSSetShader(d3d::pixel_shader.shader.Get(), nullptr, 0);
		}

		if (param::palette.dirty())
		{
			// TODO: commit
		}

		if (param::lantern.dirty())
		{
			// TODO: commit
		}

		using_shader = true;
	}

	void shader_prologue_(const std::string&)
	{
		shader_start();
	}

	void shader_epilogue_(const std::string&)
	{
		shader_end();
	}

	static void hook_vtable()
	{
		Direct3D_Device->draw_prologues["Direct3DDevice8::DrawPrimitive"].emplace_back(shader_prologue_);
		Direct3D_Device->draw_epilogues["Direct3DDevice8::DrawPrimitive"].emplace_back(shader_epilogue_);

		Direct3D_Device->draw_prologues["Direct3DDevice8::DrawIndexedPrimitive"].emplace_back(shader_prologue_);
		Direct3D_Device->draw_epilogues["Direct3DDevice8::DrawIndexedPrimitive"].emplace_back(shader_epilogue_);

		Direct3D_Device->draw_prologues["Direct3DDevice8::DrawPrimitiveUP"].emplace_back(shader_prologue_);
		Direct3D_Device->draw_epilogues["Direct3DDevice8::DrawPrimitiveUP"].emplace_back(shader_epilogue_);

		Direct3D_Device->draw_prologues["Direct3DDevice8::DrawIndexedPrimitiveUP"].emplace_back(shader_prologue_);
		Direct3D_Device->draw_epilogues["Direct3DDevice8::DrawIndexedPrimitiveUP"].emplace_back(shader_epilogue_);

		Direct3D_Device->make_cbuffer(param::palette, param::palette_cbuffer);
		Direct3D_Device->make_cbuffer(param::lantern, param::lantern_cbuffer);
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

	// ReSharper disable once CppDeclaratorNeverUsed
	static void __cdecl CreateDirect3DDevice_c(int behavior, int type)
	{
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
			initialized = true;
			hook_vtable();
			d3d::load_shader();
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

	#if 0
		param::WorldMatrix = WorldMatrix;

		auto wvMatrix = float4x4(WorldMatrix) * float4x4(ViewMatrix);
		param::wvMatrix = wvMatrix;

		D3DXMatrixInverse(&wvMatrix, nullptr, &wvMatrix);
		D3DXMatrixTranspose(&wvMatrix, &wvMatrix);
		// The inverse transpose matrix is used for environment mapping.
		param::wvMatrixInvT = wvMatrix;
	#endif
	}

	static void __stdcall Direct3D_SetProjectionMatrix_r(float hfov, float nearPlane, float farPlane)
	{
		TARGET_DYNAMIC(Direct3D_SetProjectionMatrix)(hfov, nearPlane, farPlane);

	#if 0
		// The view matrix can also be set here if necessary.
		param::ProjectionMatrix = float4x4(ProjectionMatrix) * float4x4(TransformationMatrix);
		param::ViewPosition = float3(InverseViewMatrix._41, InverseViewMatrix._42, InverseViewMatrix._43);
	#endif
	}

	static void __cdecl Direct3D_SetViewportAndTransform_r()
	{
		const auto original = TARGET_DYNAMIC(Direct3D_SetViewportAndTransform);

		bool invalid = TransformAndViewportInvalid != 0;
		original();

	#if 0
		if (invalid)
		{
			param::ProjectionMatrix = float4x4(ProjectionMatrix) * float4x4(TransformationMatrix);
		}
	#endif
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

		d3d::set_flags(LanternShaderFlags_Light, true);
		globals::palettes.set_palettes(type, 0);
		set_light_parameters();
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

	static auto __stdcall SetTransformHijack(Direct3DDevice8* _device, D3DTRANSFORMSTATETYPE type, float4x4* matrix)
	{
	#if 0
		param::ProjectionMatrix = *matrix;
	#endif
		return _device->SetTransform(type, matrix);
	}
#pragma endregion
}

namespace d3d
{
	//IDirect3DDevice9* device = nullptr;
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
			param::lantern.diffuse_override = false;
		}

		if (LanternInstance::specular_override_is_temp)
		{
			LanternInstance::specular_override = false;
		}

		if (apiconfig::override_light_dir)
		{
			param::lantern.light_direction = local::last_light_dir;
			apiconfig::override_light_dir = false;
		}

		if (apiconfig::alpha_ref_is_temp)
		{
			// TODO: param::AlphaRef = apiconfig::alpha_ref_value;
			apiconfig::alpha_ref_is_temp = false;
		}

		param::lantern.force_default_diffuse = false;
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
		return !vertex_shader.has_value() || pixel_shader.has_value();
	}

	void init_trampolines()
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

		WriteJump(reinterpret_cast<void*>(0x0077EE45), DrawMeshSetBuffer_asm);

		// Hijacking a IDirect3DDevice8::SetTransform call in Direct3D_SetNearFarPlanes
		// to update the projection matrix.
		// This nops:
		// mov ecx, [eax] (device)
		// call dword ptr [ecx+94h] (device->SetTransform)
		WriteData<8>(reinterpret_cast<void*>(0x00403234), 0x90i8);
		//WriteCall(reinterpret_cast<void*>(0x00403236), SetTransformHijack);
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
		//param::release_parameters();
		free_shaders();
	}
}
