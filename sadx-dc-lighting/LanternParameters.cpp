#include "stdafx.h"
#include "LanternParameters.h"

bool LanternParameters::dirty() const
{
	return normal_scale.dirty() ||
	       light_direction.dirty() ||
	       allow_vcolor.dirty() ||
	       diffuse_override.dirty() ||
	       diffuse_override_color.dirty() ||
	       force_default_diffuse.dirty();
}

void LanternParameters::clear()
{
	normal_scale.clear();
	light_direction.clear();
	allow_vcolor.clear();
	diffuse_override.clear();
	diffuse_override_color.clear();
	force_default_diffuse.clear();
}

void LanternParameters::mark()
{
	normal_scale.mark();
	light_direction.mark();
	allow_vcolor.mark();
	diffuse_override.mark();
	diffuse_override_color.mark();
	force_default_diffuse.mark();
}

void LanternParameters::write(CBufferBase& cbuf) const
{
	cbuf << normal_scale
	     << light_direction
	     << allow_vcolor
	     << diffuse_override
	     << diffuse_override_color
	     << force_default_diffuse;
}
