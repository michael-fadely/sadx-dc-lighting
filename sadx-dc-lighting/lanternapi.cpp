#include "stdafx.h"

#include <algorithm>

#include "lantern.h"
#include "globals.h"
#include "d3d.h"

#include "../include/lanternapi.h"
#include "apiconfig.h"

inline void check_blend()
{
	if (param::PaletteB.value() == nullptr)
	{
		param::PaletteB = param::PaletteA;
	}
}

void pl_load_register(lantern_load_cb callback)
{
	globals::palettes.add_pl_callback(callback);
}

void pl_load_unregister(lantern_load_cb callback)
{
	globals::palettes.remove_pl_callback(callback);
}

void sl_load_register(lantern_load_cb callback)
{
	globals::palettes.add_sl_callback(callback);
}

void sl_load_unregister(lantern_load_cb callback)
{
	globals::palettes.remove_sl_callback(callback);
}

void material_register(NJS_MATERIAL const* const* materials, size_t length, lantern_material_cb callback)
{
	if (!length || materials == nullptr || callback == nullptr)
	{
		return;
	}

	for (size_t i = 0; i < length; i++)
	{
		auto material = materials[i];
		auto it = apiconfig::material_callbacks.find(material);

		if (it == apiconfig::material_callbacks.end())
		{
			apiconfig::material_callbacks[material] = { callback };
		}
		else
		{
			it->second.push_front(callback);
		}
	}
}

void material_unregister(NJS_MATERIAL const* const* materials, size_t length, lantern_material_cb callback)
{
	if (!length || materials == nullptr || callback == nullptr)
	{
		return;
	}

	for (size_t i = 0; i < length; i++)
	{
		auto it = apiconfig::material_callbacks.find(materials[i]);

		if (it == apiconfig::material_callbacks.end())
		{
			continue;
		}

		auto& callbacks = it->second;
		callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback));

		if (it->second.empty())
		{
			apiconfig::material_callbacks.erase(it);
		}
	}
}

void set_shader_flags(uint32_t flags, bool add)
{
	d3d::set_flags(flags, add);
}

void set_diffuse(int32_t n, bool permanent)
{
	globals::palettes.diffuse_index(n);
	LanternInstance::diffuse_override = n >= 0;
	LanternInstance::diffuse_override_is_temp = !permanent;
}

void set_specular(int32_t n, bool permanent)
{
	globals::palettes.specular_index(n);
	LanternInstance::specular_override = n >= 0;
	LanternInstance::specular_override_is_temp = !permanent;
}

int32_t get_diffuse()
{
	return (!globals::palettes.size()) ? -1 : globals::palettes[0].diffuse_index();
}

int32_t get_specular()
{
	return (!globals::palettes.size()) ? -1 : globals::palettes[0].specular_index();
}

void set_blend_factor(float factor)
{
	set_diffuse_blend_factor(factor);
	set_specular_blend_factor(factor);
}

void allow_object_vcolor(bool allow)
{
	apiconfig::object_vcolor = allow;
}

void use_default_diffuse(bool use)
{
	param::ForceDefaultDiffuse = use;
}

void diffuse_override(bool enable)
{
	param::DiffuseOverride = enable;
}

void diffuse_override_rgb(float r, float g, float b)
{
	const D3DXVECTOR3 color = { r, g, b };
	param::DiffuseOverrideColor = color;
}

void set_diffuse_blend(int32_t src, int32_t dest)
{
	if (dest < -1 || dest > 7)
	{
		return;
	}

	check_blend();

	if (src == -1)
	{
		globals::palettes.diffuse_blend_all(dest);
		return;
	}

	if (src < 0 || src > 7)
	{
		return;
	}

	globals::palettes.diffuse_blend(src, dest);
}

void set_specular_blend(int32_t src, int32_t dest)
{
	if (dest < -1 || dest > 7)
	{
		return;
	}

	check_blend();

	if (src == -1)
	{
		globals::palettes.specular_blend_all(dest);
		return;
	}

	if (src < 0 || src > 7)
	{
		return;
	}

	globals::palettes.specular_blend(src, dest);
}

int32_t get_diffuse_blend(int32_t src)
{
	if (src < 0 || src > 7)
	{
		return -1;
	}

	return globals::palettes.diffuse_blend(src);
}

int32_t get_specular_blend(int32_t src)
{
	if (src < 0 || src > 7)
	{
		return -1;
	}

	return globals::palettes.specular_blend(src);
}

void set_diffuse_blend_factor(float factor)
{
	check_blend();
	LanternInstance::diffuse_blend_factor(factor);
}

void palette_from_rgb(int index, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	globals::palettes.palette_from_rgb(index, r, g, b, specular, apply);
}

void palette_from_array(int index, NJS_ARGB* colors, bool specular, bool apply)
{
	globals::palettes.palette_from_array(index, colors, specular, apply);
}

void palette_from_mix(int index, int index_source, Uint8 r, Uint8 g, Uint8 b, bool specular, bool apply)
{
	globals::palettes.palette_from_mix(index, index_source, r, g, b, specular, apply);
}

void set_specular_blend_factor(float factor)
{
	check_blend();
	LanternInstance::specular_blend_factor(factor);
}

float get_diffuse_blend_factor()
{
	return LanternInstance::diffuse_blend_factor();
}

float get_specular_blend_factor()
{
	return LanternInstance::specular_blend_factor();
}

void set_blend(int32_t src, int32_t dest)
{
	set_diffuse_blend(src, dest);
	set_specular_blend(src, dest);
}

void set_alpha_reject(float threshold, bool permanent)
{
	if (!permanent)
	{
		if (!apiconfig::alpha_ref_is_temp)
		{
			apiconfig::alpha_ref_value = param::AlphaRef.value();
			apiconfig::alpha_ref_is_temp = true;
		}
	}
	else
	{
		apiconfig::alpha_ref_value = threshold;
	}

	param::AlphaRef = threshold;
}

float get_alpha_reject()
{
	return param::AlphaRef.value();
}

void set_light_direction(const NJS_VECTOR* v)
{
	if (v != nullptr)
	{
		apiconfig::override_light_dir = true;
		apiconfig::light_dir_override = *v;
	}
}
