// Based on FixedFuncEMU.fx
// Copyright (c) 2005 Microsoft Corporation. All rights reserved.
//

#define FOGMODE_NONE   0
#define FOGMODE_EXP    1
#define FOGMODE_EXP2   2
#define FOGMODE_LINEAR 3
#define E 2.71828

#define D3DMCS_MATERIAL 0 // Color from material is used
#define D3DMCS_COLOR1   1 // Diffuse vertex color is used
#define D3DMCS_COLOR2   2 // Specular vertex color is used

#define PI 3.141592

struct VS_IN
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 tex      : TEXCOORD0;
	float4 color    : COLOR0;
};

struct PS_IN
{
	float4 position : POSITION0;
	float4 diffuse  : COLOR0;
	float4 specular : COLOR1;
	float2 tex      : TEXCOORD0;
	float  fogDist  : FOG;
};

Texture2D BaseTexture;
Texture2D DiffusePalette;
Texture2D SpecularPalette;

sampler2D baseSampler = sampler_state
{
	Texture = <BaseTexture>;
};

sampler2D diffuseSampler = sampler_state
{
	Texture   = <DiffusePalette>;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU  = Clamp;
	AddressV  = Clamp;
};

sampler2D specularSampler = sampler_state
{
	Texture   = <SpecularPalette>;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU  = Clamp;
	AddressV  = Clamp;
};

float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjectionMatrix;
// The inverse transpose of the world view matrix - used for environment mapping.
float4x4 wvMatrixInvT;
// Used primarily for environment mapping.
// TODO: check if texture transform is enabled for standard textures
float4x4 TextureTransform = {
	-0.5, 0.0, 0.0, 0.0,
	 0.0, 0.5, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.5, 0.5, 0.0, 1.0
};

bool TextureEnabled    = true;
bool UseVertexColor    = true;
bool EnvironmentMapped = false;
bool AlphaEnabled      = true;

uint   FogMode = (uint)FOGMODE_NONE;
float  FogStart;
float  FogEnd;
float  FogDensity;
float4 FogColor;

float3 LightDirection  = float3(0.0f, -1.0f, 0.0f);
float  LightLength     = 1.0f;
uint   DiffuseSource   = (uint)D3DMCS_COLOR1;
float4 MaterialDiffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
float  AlphaRef        = 16.0f / 255.0f;

// Helper functions

float CalcFogFactor(float d)
{
	if (FogMode == FOGMODE_NONE)
		return 1;

	float fogCoeff;

	switch (FogMode)
	{
		default:
			break;

		case FOGMODE_EXP:
			fogCoeff = 1.0 / pow(E, d * FogDensity);
			break;

		case FOGMODE_EXP2:
			fogCoeff = 1.0 / pow(E, d * d * FogDensity * FogDensity);
			break;
		case FOGMODE_LINEAR:
			fogCoeff = (FogEnd - d) / (FogEnd - FogStart);
			break;
	}

	return clamp(fogCoeff, 0, 1);
}
inline float4 GetDiffuse(in float4 vcolor)
{
	if (DiffuseSource == D3DMCS_MATERIAL)
	{
		return MaterialDiffuse;
	}

	return any(vcolor) && UseVertexColor ? vcolor : MaterialDiffuse;
}
inline void TransformF(out float4 output, in float3 input, out float fogDist)
{
	float4 wvPos = mul(mul(float4(input, 1), WorldMatrix), ViewMatrix);
	output = mul(wvPos, ProjectionMatrix);
	fogDist = wvPos.z;
}
inline void Transform(out float4 output, in float3 input)
{
	output = mul(float4(input, 1), WorldMatrix);
	output = mul(output, ViewMatrix);
	output = mul(output, ProjectionMatrix);
}
inline void EnvironmentMap(out float2 tex, in float2 itex, in float3 normal)
{
	if (TextureEnabled && EnvironmentMapped)
	{
		tex = (float2)mul(float4(normal, 1), wvMatrixInvT);
		tex = (float2)mul(float4(tex, 0, 1), TextureTransform);
	}
	else
	{
		tex = itex;
	}
}
inline void DoLighting(in float4 color, in float3 normal, out float4 diffuse, out float4 specular)
{
	float3 worldNormal = normalize(mul(normal, (float3x3)WorldMatrix));

	// This is the brightness index of the current vertex's normal.
	// It's calculated the same way one would calculate diffuse brightness, except it's then
	// converted to an angle, and then to an index from there. 0 is brightest, 1 is darkest.
	float i = +acos(dot(LightDirection, worldNormal) / (LightLength * length(worldNormal))) / PI;

	// Specifically avoiding the alpha component here. Palettes don't seem to do anything
	// useful with their alpha channels.
	diffuse = GetDiffuse(color);
	diffuse.rgb = saturate(diffuse + 0.3) * tex2Dlod(diffuseSampler, float4(0, i, 0, 0));

	// Ditto. You wouldn't want to add to the alpha channel.
	specular = tex2Dlod(specularSampler, float4(0, pow(i, 1.5), 0, 0));
	specular.a = 0;
}
inline void NoLighting(in float4 color, out float4 diffuse, out float4 specular)
{
	// Just spit out the vertex or material color if lighting is off.
	diffuse = GetDiffuse(color);
	specular = 0;
}
inline void ApplyFog(float factor, inout float4 result)
{
	result.rgb = (float3)(factor * result + (1.0 - factor) * FogColor);
}
inline void CheckAlpha(in float4 color)
{
	if (!AlphaEnabled)
		return;

	clip(color.a < AlphaRef ? -1 : 1);
}
inline float4 Blend(in float2 uv, in float4 diffuse, in float4 specular)
{
	if (TextureEnabled)
	{
		float4 base = tex2D(baseSampler, uv);
		return base * diffuse + specular;
	}
	else
	{
		return diffuse + specular;
	}
}

// Vertex shaders

PS_IN vs_main(VS_IN input)
{
	PS_IN output;
	TransformF(output.position, input.position, output.fogDist);
	EnvironmentMap(output.tex, input.tex, input.normal);
	DoLighting(input.color, input.normal, output.diffuse, output.specular);
	return output;
}
PS_IN vs_nolight(VS_IN input)
{
	PS_IN output;
	TransformF(output.position, input.position, output.fogDist);
	EnvironmentMap(output.tex, input.tex, input.normal);
	NoLighting(input.color, output.diffuse, output.specular);
	return output;
}
PS_IN vs_nofog(VS_IN input)
{
	PS_IN output;
	output.fogDist = 0;
	Transform(output.position, input.position);
	EnvironmentMap(output.tex, input.tex, input.normal);
	DoLighting(input.color, input.normal, output.diffuse, output.specular);
	return output;
}
PS_IN vs_neither(VS_IN input)
{
	PS_IN output;
	output.fogDist = 0;
	Transform(output.position, input.position);
	EnvironmentMap(output.tex, input.tex, input.normal);
	NoLighting(input.color, output.diffuse, output.specular);
	return output;
}

// Pixel shaders

float4 ps_main(PS_IN input) : COLOR
{
	float4 result = Blend(input.tex, input.diffuse, input.specular);
	CheckAlpha(result);
	ApplyFog(CalcFogFactor(input.fogDist), result);
	return result;
}
float4 ps_nolight(PS_IN input) : COLOR
{
	float4 result = Blend(input.tex, input.diffuse, input.specular);
	CheckAlpha(result);
	ApplyFog(CalcFogFactor(input.fogDist), result);
	return result;
}
float4 ps_nofog(PS_IN input) : COLOR
{
	float4 result = Blend(input.tex, input.diffuse, input.specular);
	CheckAlpha(result);
	return result;
}
float4 ps_neither(PS_IN input) : COLOR
{
	float4 result = Blend(input.tex, input.diffuse, input.specular);
	CheckAlpha(result);
	return result;
}

// Techniques

// TODO: Benchmark these different techniques vs single technique with boolean checks.

technique Standard
{
	pass
	{
		VertexShader = compile vs_3_0 vs_main();
		PixelShader = compile ps_3_0 ps_main();
	}
}

technique NoLight
{
	pass
	{
		VertexShader = compile vs_3_0 vs_nolight();
		PixelShader = compile ps_3_0 ps_nolight();
	}
}

technique NoFog
{
	pass
	{
		VertexShader = compile vs_3_0 vs_nofog();
		PixelShader = compile ps_3_0 ps_nofog();
	}
}

technique Neither
{
	pass
	{
		VertexShader = compile vs_3_0 vs_neither();
		PixelShader = compile ps_3_0 ps_neither();
	}
}
