#include "stdafx.h"

#include <algorithm>

#include "lantern.h"
#include "globals.h"
#include "d3d.h"

#include "../include/lanternapi.h"

using namespace globals;

inline void check_blend()
{
	if (param::PaletteB.Value() == nullptr)
	{
		param::PaletteB = param::PaletteA;
	}
}

void pl_load_register(lantern_load_cb callback)
{
	palettes.AddPlCallback(callback);
}

void pl_load_unregister(lantern_load_cb callback)
{
	palettes.RemovePlCallback(callback);
}

void sl_load_register(lantern_load_cb callback)
{
	palettes.AddSlCallback(callback);
}

void sl_load_unregister(lantern_load_cb callback)
{
	palettes.RemoveSlCallback(callback);
}

void material_register(NJS_MATERIAL** materials, int length, lantern_material_cb callback)
{
	if (length < 1 || materials == nullptr || callback == nullptr)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		auto material = materials[i];
		auto it = material_callbacks.find(material);

		if (it == material_callbacks.end())
		{
			material_callbacks[material] = { callback };
		}
		else
		{
			it->second.push_front(callback);
		}
	}
}

void material_unregister(NJS_MATERIAL** materials, int length, lantern_material_cb callback)
{
	if (length < 1 || materials == nullptr || callback == nullptr)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		auto it = material_callbacks.find(materials[i]);

		if (it == material_callbacks.end())
		{
			continue;
		}

		remove(it->second.begin(), it->second.end(), callback);

		if (it->second.empty())
		{
			material_callbacks.erase(it);
		}
	}
}

void set_shader_flags(unsigned int flags, bool add)
{
	d3d::SetShaderFlags(flags, add);
}

void allow_landtable_specular(bool allow)
{
	landtable_specular = allow;
}

void set_diffuse(int n, bool permanent)
{
	palettes.DiffuseIndex(n);
	LanternInstance::diffuse_override = n >= 0;
	LanternInstance::diffuse_override_temp = !permanent;
}

void set_specular(int n, bool permanent)
{
	palettes.SpecularIndex(n);
	LanternInstance::specular_override = n >= 0;
	LanternInstance::specular_override_temp = !permanent;
}

int get_diffuse()
{
	return (!palettes.Size()) ? -1 : palettes[0].DiffuseIndex();
}

int get_specular()
{
	return (!palettes.Size()) ? -1 : palettes[0].SpecularIndex();
}

void set_blend_factor(float factor)
{
	set_diffuse_blend_factor(factor);
	set_specular_blend_factor(factor);
}

void allow_object_vcolor(bool allow)
{
	object_vcolor = allow;
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
	D3DXVECTOR3 color = { r, g, b };
	param::DiffuseOverrideColor = color;
}

void set_diffuse_blend(int src, int dest)
{
	if (dest < -1 || dest > 7)
	{
		return;
	}

	check_blend();

	if (src == -1)
	{
		palettes.DiffuseBlendAll(dest);
		return;
	}

	if (src < 0 || src > 7)
	{
		return;
	}

	palettes.DiffuseBlend(src, dest);
}

void set_specular_blend(int src, int dest)
{
	if (dest < -1 || dest > 7)
	{
		return;
	}

	check_blend();

	if (src == -1)
	{
		palettes.SpecularBlendAll(dest);
		return;
	}

	if (src < 0 || src > 7)
	{
		return;
	}

	palettes.SpecularBlend(src, dest);
}

int get_diffuse_blend(int src)
{
	if (src < 0 || src > 7)
	{
		return -1;
	}

	return palettes.DiffuseBlend(src);
}

int get_specular_blend(int src)
{
	if (src < 0 || src > 7)
	{
		return -1;
	}

	return palettes.SpecularBlend(src);
}

void set_diffuse_blend_factor(float factor)
{
	check_blend();
	LanternInstance::DiffuseBlendFactor(factor);
}

void set_specular_blend_factor(float factor)
{
	check_blend();
	LanternInstance::SpecularBlendFactor(factor);
}

float get_diffuse_blend_factor()
{
	return LanternInstance::DiffuseBlendFactor();
}

float get_specular_blend_factor()
{
	return LanternInstance::SpecularBlendFactor();
}

void set_blend(int src, int dest)
{
	set_diffuse_blend(src, dest);
	set_specular_blend(src, dest);
}
