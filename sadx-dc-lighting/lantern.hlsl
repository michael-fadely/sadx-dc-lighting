#define NODE_WRITE
#include "d3d8to11.hlsl"

// From FixedFuncEMU.fx
// Copyright (c) 2005 Microsoft Corporation. All rights reserved.
#define FOGMODE_NONE   0
#define FOGMODE_EXP    1
#define FOGMODE_EXP2   2
#define FOGMODE_LINEAR 3
#define E 2.71828

#define D3DMCS_MATERIAL 0 // Color from material is used
#define D3DMCS_COLOR1   1 // Diffuse vertex color is used
#define D3DMCS_COLOR2   2 // Specular vertex color is used

#define DEFAULT_SAMPLER     \
	MinFilter = Point;      \
	MagFilter = Point;      \
	AddressU  = Clamp;      \
	AddressV  = Clamp;      \
	ColorOp   = Modulate;   \
	ColorArg1 = Texture;    \
	ColorArg2 = Current;    \
	AlphaOp   = SelectArg1; \
	AlphaArg1 = Texture;    \
	AlphaArg2 = Current

Texture2D palette_a : register(t8);
Texture2D palette_b : register(t9);

SamplerState palette_sampler_a : register(s8);
SamplerState palette_sampler_b : register(s9);

struct PaletteIndexPair
{
	int diffuse;
	int specular;
};

struct PaletteBlendFactorPair
{
	float diffuse;
	float specular;
};

cbuffer PaletteParameters : register(b4)
{
	PaletteIndexPair       base_indices;
	PaletteIndexPair       blend_indices;
	PaletteBlendFactorPair blend_factors;
};

cbuffer LanternParameters : register(b5)
{
	float3 normal_scale;
	float3 light_direction;
	bool   allow_vcolor;
	bool   diffuse_override;
	float4 diffuse_override_color;
	bool   force_default_diffuse;
};

// Helpers

float4 GetDiffuse(in float4 vcolor)
{
	float4 color = (material_sources.diffuse == D3DMCS_COLOR1 && any(vcolor)) ? vcolor : material.diffuse;

	if (!allow_vcolor || force_default_diffuse)
	{
		return float4(1, 1, 1, color.a);
	}

	return color;
}

// Vertex shaders

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output = fixed_func_vs(input);

#ifdef FVF_DIFFUSE
	float4 input_diffuse = input.diffuse;
#else
	float4 input_diffuse = (float4)0;
#endif

#if defined(USE_LIGHT) && defined(FVF_NORMAL)
	{
		float3 worldNormal = mul(input.normal * normal_scale, (float3x3)world_matrix);
		float4 diffuse = GetDiffuse(input_diffuse);

		// This is the "brightness index" calculation. Just a dot product
		// of the vertex normal (in world space) and the light direction.
		float _dot = dot(normalize(light_direction), worldNormal);

		// The palette's brightest point is 0, and its darkest point is 1,
		// so we push the dot product (-1 .. 1) into the rage 0 .. 1, and
		// subtract it from 1. This is the value we use for indexing into
		// the palette.
		// HACK: This clamp prevents a visual bug in the Mystic Ruins past (shrine on fire)
		int i = floor(clamp(1 - (_dot + 1) / 2, 0, 0.99) * 255);

		float4 pdiffuse;

		if (diffuse_override)
		{
			pdiffuse = float4(diffuse_override_color.rgb, 1);
		}
		else
		{
			pdiffuse = palette_a[int2(i, base_indices.diffuse)];
		}

		float4 pspecular = palette_a[int2(i, base_indices.specular)];

	#ifdef USE_BLEND
		{
			float4 bdiffuse  = palette_b[int2(i, blend_indices.diffuse)];
			float4 bspecular = palette_b[int2(i, blend_indices.specular)];

			pdiffuse = lerp(pdiffuse, bdiffuse, blend_factors.diffuse);
			pspecular = lerp(pspecular, bspecular, blend_factors.specular);
		}
	#endif

		output.diffuse = float4((diffuse * pdiffuse).rgb, diffuse.a);
		output.specular = float4(pspecular.rgb, 0.0f);
	}
#else
	{
		// Just spit out the vertex or material color if lighting is off.
		output.diffuse = GetDiffuse(input_diffuse);
		output.specular = 0;
	}
#endif

	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET
{
	float4 result;

#ifdef USE_TEXTURE
	result = textures[0].Sample(samplers[0], input.uv[0].xy);
	result = result * input.diffuse + input.specular;
#else
	result = input.diffuse;
#endif

	result = apply_fog(result, input.fog);

	bool standard_blending = is_standard_blending();

	do_alpha_reject(result, standard_blending);
	do_oit(result, input, standard_blending);

	return result;
}
