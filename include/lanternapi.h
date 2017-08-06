#ifndef _LANTERNAPI_H
#define _LANTERNAPI_H

#include <ninja.h>

#ifdef LANTERN_API
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum
	{
		ShaderFlags_None,
		ShaderFlags_Texture = 1 << 0,
		ShaderFlags_EnvMap  = 1 << 1,
		ShaderFlags_Alpha   = 1 << 2,
		ShaderFlags_Light   = 1 << 3,
		ShaderFlags_Blend   = 1 << 4,
		ShaderFlags_Fog     = 1 << 5,
		ShaderFlags_Mask    = 0x3F,
		ShaderFlags_Count
	} ShaderFlags;

	typedef const char* (__cdecl* lantern_load_cb)(int level, int act);
	typedef bool (__cdecl* lantern_material_cb)(NJS_MATERIAL* material, Uint32 flags);

	API void pl_load_register(lantern_load_cb callback);
	API void pl_load_unregister(lantern_load_cb callback);
	API void sl_load_register(lantern_load_cb callback);
	API void sl_load_unregister(lantern_load_cb callback);

	API void material_register(NJS_MATERIAL** materials, int length, lantern_material_cb callback);
	API void material_unregister(NJS_MATERIAL** materials, int length, lantern_material_cb callback);

	API void set_shader_flags(unsigned int flags, bool add);

	API void allow_landtable_specular(bool allow);
	API void allow_object_vcolor(bool allow);
	API void use_default_diffuse(bool use);

	API void set_diffuse(int n, bool permanent);
	API void set_specular(int n, bool permanent);

	API void diffuse_override(bool enable);
	API void diffuse_override_rgb(float r, float g, float b);

	API int get_diffuse();
	API int get_specular();

	API void set_diffuse_blend(int n);
	API void set_specular_blend(int n);

	API int get_diffuse_blend();
	API int get_specular_blend();

	API void set_blend_factor(float factor);
	API float get_blend_factor();

#ifdef __cplusplus
}
#endif

#endif // _LANTERNAPI_H
