#pragma once
#include <CBufferWriter.h>
#include <dirty_t.h>
#include <simple_math.h> 

struct LanternParameters : ICBuffer, dirty_impl
{
	dirty_t<float3> normal_scale;
	dirty_t<float3> light_direction;
	dirty_t<bool>   allow_vcolor;
	dirty_t<bool>   diffuse_override;
	dirty_t<float4> diffuse_override_color;
	dirty_t<bool>   force_default_diffuse;
	
	[[nodiscard]] bool dirty() const override;
	void clear() override;
	void mark() override;
	void write(CBufferBase& cbuf) const override;
};
