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

	/**
	 * Gets the currently set diffuse index.
	 */
	API int get_diffuse();

	/**
	 * Gets the currently set specular index.
	 */
	API int get_specular();

	/**
	 * \brief Enable blending from a source diffuse
	 * index to a destination diffuse index.

	 * \param src Source index in the range 0 to 7.
	 * -1 to apply to all indices.
	 * \param dest Destination index in the range 0 to 7.
	 */
	API void set_diffuse_blend(int src, int dest);

	/**
	 * \brief Enable blending from a source specular
	 * index to a destination specular index.

	 * \param src Source index in the range 0 to 7.
	 * -1 to apply to all indices.
	 * \param dest Destination index in the range 0 to 7.
	 */
	API void set_specular_blend(int src, int dest);

	/**
	 * \brief Returns the current destination blend index
	 * for the specified source diffuse index.

	 * \param src Source index in the range 0 to 7.
	 * Values outside this range will always return -1.

	 * \return A value in the range 0 to 7, or -1 if not set.
	 */
	API int get_diffuse_blend(int src);

	/**
	 * \brief Returns the current destination blend index
	 * for the specified source specular index.

	 * \param src Source index in the range 0 to 7.
	 * Values outside this range will always return -1.

	 * \return A value in the range 0 to 7, or -1 if not set.
	 */
	API int get_specular_blend(int src);

	/**
	 * \brief Set diffuse index blending factor.
	 * \param factor A blending factor in the range 0.0f to 1.0f.
	 * Behavior of values outside this range is undefined.
	 */
	API void set_diffuse_blend_factor(float factor);

	/**
	 * \brief Set specular index blending factor.
	 * \param factor A blending factor in the range 0.0f to 1.0f.
	 * Behavior of values outside this range is undefined.
	 */
	API void set_specular_blend_factor(float factor);

	/**
	 * \brief Gets the diffuse blend factor set by set_diffuse_blend_factor.
	 */
	API float get_diffuse_blend_factor();

	/**
	 * \brief Gets the specular blend factor set by set_specular_blend_factor.
	 */
	API float get_specular_blend_factor();

	/**
	 * \brief Set diffuse and specular index blending factor simultaneously.
	 * \param factor A blending factor in the range 0.0f to 1.0f.
	 * Behavior of values outside this range is undefined.

	 * \see set_diffuse_blend_factor
	 * \see set_specular_blend_factor
	 */
	API void set_blend_factor(float factor);

#ifdef __cplusplus
}
#endif

#endif // _LANTERNAPI_H
