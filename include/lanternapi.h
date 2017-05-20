#ifndef _LANTERNAPI_H
#define _LANTERNAPI_H

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

	__declspec(dllexport) void pl_load_register(lantern_load_t callback);
	__declspec(dllexport) void pl_load_unregister(lantern_load_t callback);
	__declspec(dllexport) void sl_load_register(lantern_load_t callback);
	__declspec(dllexport) void sl_load_unregister(lantern_load_t callback);

	__declspec(dllexport) void set_shader_flags(unsigned int flags, bool add);

	__declspec(dllexport) void landtable_allow_specular(bool allow);

	__declspec(dllexport) void set_diffuse(int n);
	__declspec(dllexport) void set_specular(int n);

	__declspec(dllexport) int get_diffuse();
	__declspec(dllexport) int get_specular();

	__declspec(dllexport) void set_diffuse_blend(int n);
	__declspec(dllexport) void set_specular_blend(int n);

	__declspec(dllexport) int get_diffuse_blend();
	__declspec(dllexport) int get_specular_blend();

	__declspec(dllexport) void set_blend_factor(float factor);
	__declspec(dllexport) float get_blend_factor();

#ifdef __cplusplus
}
#endif

#endif // _LANTERNAPI_H
