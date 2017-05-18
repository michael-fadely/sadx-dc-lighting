#include "stdafx.h"

#include "lantern.h"
#include "globals.h"
#include "d3d.h"

#include "lanternapi.h"

using namespace globals;

void pl_load_register(lantern_load_t callback)
{
	palettes.AddPlCallback(callback);
}

void pl_load_unregister(lantern_load_t callback)
{
	palettes.RemovePlCallback(callback);
}

void sl_load_register(lantern_load_t callback)
{
	palettes.AddSlCallback(callback);
}

void sl_load_unregister(lantern_load_t callback)
{
	palettes.RemoveSlCallback(callback);
}

void set_shader_flags(unsigned int flags, bool add)
{
	d3d::SetShaderFlags(flags, add);
}

void landtable_allow_specular(bool allow)
{
	landtable_specular = allow;
}

void set_diffuse(int n)
{
	if (palettes.Size() > 0)
	{
		palettes[0].SetDiffuse(n);
	}
}

void set_specular(int n)
{
	if (palettes.Size() > 0)
	{
		palettes[0].SetSpecular(n);
	}
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
