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
float4x4 wvMatrix;
float4x4 ProjectionMatrix;
// The inverse transpose of the world view matrix - used for environment mapping.
float4x4 wvMatrixInvT;
// Used primarily for environment mapping.
// TODO: check if texture transform is enabled for standard textures
static float4x4 TextureTransform = {
	-0.5, 0.0, 0.0, 0.0,
	 0.0, 0.5, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.5, 0.5, 0.0, 1.0
};

// This will need to be non-static for chunk models.
static uint DiffuseSource = (uint)D3DMCS_COLOR1;
// This never changes
static float  AlphaRef = 16.0f / 255.0f;

bool TextureEnabled    = true;
bool EnvironmentMapped = false;
bool AlphaEnabled      = true;

uint   FogMode = (uint)FOGMODE_NONE;
float  FogStart;
float  FogEnd;
float  FogDensity;
float4 FogColor;

float3 LightDirection  = float3(0.0f, -1.0f, 0.0f);
float  LightLength     = 1.0f;
float4 MaterialDiffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);

// Helpers

float4 GetDiffuse(in float4 vcolor)
{
	return any(vcolor) ? vcolor : MaterialDiffuse;
}
float CalcFogFactor(float d)
{
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

// Vertex shaders

PS_IN vs_main(VS_IN input, uniform bool lightEnabled)
{
	PS_IN output;

	float4 position;

	position = mul(float4(input.position, 1), wvMatrix);
	output.fogDist = position.z;
	position = mul(position, ProjectionMatrix);

	output.position = position;

	if (TextureEnabled && EnvironmentMapped)
	{
		output.tex = (float2)mul(float4(input.normal, 1), wvMatrixInvT);
		output.tex = (float2)mul(float4(output.tex, 0, 1), TextureTransform);
	}
	else
	{
		output.tex = input.tex;
	}

	if (lightEnabled == true)
	{
		float3 worldNormal = normalize(mul(input.normal, (float3x3)WorldMatrix));

		// This is the brightness index of the current vertex's normal.
		// It's calculated the same way one would calculate diffuse brightness, except it's then
		// converted to an angle, and then to an index from there. 0 is brightest, 1 is darkest.
		float i = +acos(dot(LightDirection, worldNormal) / (LightLength * length(worldNormal))) / PI;

		// Specifically avoiding the alpha component here. Palettes don't seem to do anything
		// useful with their alpha channels.
		float4 diffuse = GetDiffuse(input.color);
		diffuse.rgb = saturate(diffuse + 0.3) * tex2Dlod(diffuseSampler, float4(0, i, 0, 0));

		output.diffuse = diffuse;

		// Ditto. You wouldn't want to add to the alpha channel.
		float4 specular = tex2Dlod(specularSampler, float4(0, pow(i, 1.5), 0, 0));
		specular.a = 0;

		output.specular = specular;
	}
	else
	{
		// Just spit out the vertex or material color if lighting is off.
		output.diffuse = GetDiffuse(input.color);
		output.specular = 0;
	}

	return output;
}

PS_IN vs_light(VS_IN input)
{
	return vs_main(input, true);
}
PS_IN vs_nolight(VS_IN input)
{
	return vs_main(input, false);
}

// Pixel shaders

float4 ps_main(PS_IN input, uniform bool useFog)
{
	float4 result;

	if (TextureEnabled)
	{
		result = tex2D(baseSampler, input.tex);
		result = result * input.diffuse + input.specular;
	}
	else
	{
		result = input.diffuse + input.specular;
	}

	if (AlphaEnabled)
	{
		clip(result.a < AlphaRef ? -1 : 1);
	}

	if (useFog)
	{
		float factor = CalcFogFactor(input.fogDist);

		if (factor == 0.0f)
		{
			result.rgb = FogColor.rgb;
		}
		else
		{
			result.rgb = (float3)(factor * result + (1.0 - factor) * FogColor);
		}
	}

	return result;
}

float4 ps_fog(PS_IN input) : COLOR
{
	return ps_main(input, true);
}
float4 ps_nofog(PS_IN input) : COLOR
{
	return ps_main(input, false);
}

// Techniques

technique Standard
{
	pass
	{
		VertexShader = compile vs_3_0 vs_light();
		PixelShader  = compile ps_3_0 ps_fog();
	}
}

technique NoLight
{
	pass
	{
		VertexShader = compile vs_3_0 vs_nolight();
		PixelShader  = compile ps_3_0 ps_fog();
	}
}

technique NoFog
{
	pass
	{
		VertexShader = compile vs_3_0 vs_light();
		PixelShader  = compile ps_3_0 ps_nofog();
	}
}

technique Neither
{
	pass
	{
		VertexShader = compile vs_3_0 vs_nolight();
		PixelShader  = compile ps_3_0 ps_nofog();
	}
}
