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
sampler2D baseSampler = sampler_state
{
	Texture = <BaseTexture>;
};

Texture2D PaletteA;
sampler2D atlasSamplerA = sampler_state
{
	Texture   = <PaletteA>;
	MinFilter = Point;
	MagFilter = Point;
	AddressU  = Clamp;
	AddressV  = Clamp;
};

Texture2D PaletteB;
sampler2D atlasSamplerB = sampler_state
{
	Texture   = <PaletteB>;
	MinFilter = Point;
	MagFilter = Point;
	AddressU  = Clamp;
	AddressV  = Clamp;
};

float4x4 WorldMatrix;
float4x4 wvMatrix;
float4x4 ProjectionMatrix;
// The inverse transpose of the world view matrix - used for environment mapping.
float4x4 wvMatrixInvT;
// Used primarily for environment mapping.
float4x4 TextureTransform = {
	-0.5, 0.0, 0.0, 0.0,
	 0.0, 0.5, 0.0, 0.0,
	 0.0, 0.0, 1.0, 0.0,
	 0.5, 0.5, 0.0, 1.0
};

// This never changes
static float AlphaRef = 16.0f / 255.0f;

// Pre-adjusted on the CPU before being sent to the shader.
float DiffuseIndexA = 0;
float DiffuseIndexB = 0;
float SpecularIndexA = 0;
float SpecularIndexB = 0;

bool TextureEnabled    = true;
bool EnvironmentMapped = false;
bool AlphaEnabled      = true;

uint   FogMode = (uint)FOGMODE_NONE;
float  FogStart;
float  FogEnd;
float  FogDensity;
float4 FogColor;

float3 LightDirection   = float3(0.0f, -1.0f, 0.0f);
float4 MaterialDiffuse  = float4(1.0f, 1.0f, 1.0f, 1.0f);
uint   DiffuseSource    = (uint)D3DMCS_COLOR1;

float3 NormalScale = float3(1, 1, 1);

float BlendFactor = 0.0f;

#ifdef USE_SL

struct SourceLight_t
{
	int y, z;
	float3 ambient;
	float2 unknown;
	float power;
};

bool          UseSourceLight   = false;
float4        MaterialSpecular = float4(0.0f, 0.0f, 0.0f, 0.0f);
float         MaterialPower    = 1.0f;
SourceLight_t SourceLight;

#endif

// Helpers

float4 GetDiffuse(in float4 vcolor)
{
	float4 color = (DiffuseSource == D3DMCS_COLOR1 && any(vcolor)) ? vcolor : MaterialDiffuse;

	if (floor(color.r * 255) == 178
		&& floor(color.g * 255) == 178
		&& floor(color.b * 255) == 178)
	{
		return float4(1, 1, 1, color.a);
	}

	return color;
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

PS_IN vs_main(VS_IN input, uniform bool lightEnabled, uniform bool interpolate = true)
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
		float3 worldNormal = mul(input.normal * NormalScale, (float3x3)WorldMatrix);

		// This is the "brightness index" calculation. Just a dot product
		// of the vertex normal (in world space) and the light direction.
		float _dot = dot(LightDirection, worldNormal);
		float4 diffuse = GetDiffuse(input.color);

		// The palette's brightest point is 0, and its darkest point is 1,
		// so we push the dot product (-1 .. 1) into the rage 0 .. 1, and
		// subtract it from 1. This is the value we use for indexing into
		// the palette.
		// HACK: This clamp prevents a visual bug in the Mystic Ruins past (shrine on fire)
		float i = floor(clamp(1 - (_dot + 1) / 2, 0, 0.99) * 255) / 255;

#ifdef USE_SL
		if (UseSourceLight == true)
		{
			output.diffuse = max(0, float4(diffuse.rgb * SourceLight.ambient * _dot, diffuse.a));
			output.specular = max(0, float4(1, 1, 1, 0.0f) * pow(_dot, MaterialPower));
		}
		else
		{
#endif
			float4 pdiffuse = tex2Dlod(atlasSamplerA, float4(i, DiffuseIndexA, 0, 0));
			float4 pspecular = tex2Dlod(atlasSamplerA, float4(i, SpecularIndexA, 0, 0));

			if (interpolate)
			{
				float4 bdiffuse = tex2Dlod(atlasSamplerB, float4(i, DiffuseIndexB, 0, 0));
				float4 bspecular = tex2Dlod(atlasSamplerB, float4(i, SpecularIndexB, 0, 0));

				pdiffuse = lerp(pdiffuse, bdiffuse, BlendFactor);
				pspecular = lerp(pspecular, bspecular, BlendFactor);
			}

			output.diffuse = float4((diffuse * pdiffuse).rgb, diffuse.a);
			output.specular = float4(pspecular.rgb, 0.0f);
#ifdef USE_SL
		}
#endif
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
	return vs_main(input, true, true);
}
PS_IN vs_nolight(VS_IN input)
{
	return vs_main(input, false, true);
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
		result.rgb = (factor * result + (1.0 - factor) * FogColor).rgb;
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

// Standard techniques

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
