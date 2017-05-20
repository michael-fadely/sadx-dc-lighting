#ifndef _LANTERNAPI_H
#define _LANTERNAPI_H

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

	typedef const char* (__cdecl* lantern_load_t)(int, int);

	API void pl_load_register(lantern_load_t callback);
	API void pl_load_unregister(lantern_load_t callback);
	API void sl_load_register(lantern_load_t callback);
	API void sl_load_unregister(lantern_load_t callback);

	API void set_shader_flags(unsigned int flags, bool add);

	API void landtable_allow_specular(bool allow);

	API void set_diffuse(int n);
	API void set_specular(int n);

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
