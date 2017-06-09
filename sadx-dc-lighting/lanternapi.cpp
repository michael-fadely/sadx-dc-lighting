#include "stdafx.h"

#include "lantern.h"
#include "globals.h"
#include "d3d.h"

#include "../include/lanternapi.h"

using namespace globals;

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
			it->second.push_back(callback);
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
	if (palettes.Size() > 0)
	{
		palettes[0].SetDiffuse(n);
	}

	LanternInstance::diffuse_override = n >= 0;
	LanternInstance::diffuse_override_temp = !permanent;
}

void set_specular(int n, bool permanent)
{
	if (palettes.Size() > 0)
	{
		palettes[0].SetSpecular(n);
	}

	LanternInstance::specular_override = n >= 0;
	LanternInstance::specular_override_temp = !permanent;
}

int get_diffuse()
{
	return (!palettes.Size()) ? -1 : palettes[0].GetDiffuse();
}

int get_specular()
{
	return (!palettes.Size()) ? -1 : palettes[0].GetSpecular();
}

void set_diffuse_blend(int n)
{
	if (palettes.Size() > 0)
	{
		palettes[0].SetDiffuseB(n);
	}
}

void set_specular_blend(int n)
{
	if (palettes.Size() > 0)
	{
		palettes[0].SetSpecularB(n);
	}
}

int get_diffuse_blend()
{
	return (!palettes.Size()) ? -1 : palettes[0].GetDiffuseB();
}

int get_specular_blend()
{
	return (!palettes.Size()) ? -1 : palettes[0].GetSpecularB();
}

void set_blend_factor(float factor)
{
	LanternInstance::SetBlendFactor(factor);
}

float get_blend_factor()
{
	return LanternInstance::BlendFactor();
}

void allow_object_vcolor(bool allow)
{
	object_vcolor = allow;
}

void use_default_diffuse(bool use)
{
	param::ForceDefaultDiffuse = use;
}
