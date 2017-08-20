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
	float  fogDist : FOG;
};

#ifdef USE_SL
struct SourceLight_t
{
	int y, z;
	float3 color;
	float specular;
	float diffuse;
	float ambient;
	float unknown2[15];
};

struct StageLight
{
	float3 direction;
	float specular;
	float multiplier;
	float3 diffuse;
	float3 ambient;
	float padding[5];
};
#endif

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

// This never changes
static const float AlphaRef = 16.0f / 255.0f;

// Diffuse texture
Texture2D BaseTexture : register(t0);
// Palette atlas A
Texture2D PaletteA : register(t1);
// Palette atlas B
Texture2D PaletteB : register(t2);

float4x4 WorldMatrix : register(c0);
float4x4 wvMatrix : register(c4);
float4x4 ProjectionMatrix : register(c8);
// The inverse transpose of the world view matrix - used for environment mapping.
float4x4 wvMatrixInvT : register(c12);

// Used primarily for environment mapping.
float4x4 TextureTransform : register(c16) = {
	-0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
};

uint DiffuseSource : register(c20) = (uint)D3DMCS_COLOR1;
float4 MaterialDiffuse : register(c21) = float4(1.0f, 1.0f, 1.0f, 1.0f);

// Pre-adjusted on the CPU before being sent to the shader.
// Used for sampling colors from the palette atlases.
float DiffuseIndexA : register(c22) = 0;
float DiffuseIndexB : register(c23) = 0;
float SpecularIndexA : register(c24) = 0;
float SpecularIndexB : register(c25) = 0;

float3 LightDirection : register(c26) = float3(0.0f, -1.0f, 0.0f);
float3 NormalScale : register(c27) = float3(1, 1, 1);

uint   FogMode : register(c28) = (uint)FOGMODE_NONE;
float  FogStart : register(c29);
float  FogEnd : register(c30);
float  FogDensity : register(c31);
float4 FogColor : register(c32);

float DiffuseBlendFactor : register(c33) = 0.0f;
float SpecularBlendFactor : register(c34) = 0.0f;

bool AllowVertexColor : register(c35) = true;
bool ForceDefaultDiffuse : register(c36) = false;
bool DiffuseOverride : register(c37) = false;
float3 DiffuseOverrideColor : register(c38) = float3(1, 1, 1);

#ifdef USE_SL
float4 MaterialSpecular : register(c39) = float4(0.0f, 0.0f, 0.0f, 0.0f);
float  MaterialPower : register(c40) = 1.0f;
SourceLight_t SourceLight : register(c41);
StageLight Lights[4] : register(c42);
#endif

// Samplers
SamplerState baseSampler : register(s0)= sampler_state
{
	Texture = BaseTexture;
};

#define DEFAULT_SAMPLER \
	MinFilter = Point;\
	MagFilter = Point;\
	AddressU  = Clamp;\
	AddressV  = Clamp;\
	ColorOp   = Modulate;\
	ColorArg1 = Texture;\
	ColorArg2 = Current;\
	AlphaOp   = SelectArg1;\
	AlphaArg1 = Texture;\
	AlphaArg2 = Current


SamplerState atlasSamplerA : register(s1) = sampler_state
{
	Texture = PaletteA;
	DEFAULT_SAMPLER;
};

SamplerState atlasSamplerB : register(s2) = sampler_state
{
	Texture = PaletteB;
	DEFAULT_SAMPLER;
};

// Helpers

#ifdef USE_FOG
// From FixedFuncEMU.fx
// Copyright (c) 2005 Microsoft Corporation. All rights reserved.

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
#endif

float4 GetDiffuse(in float4 vcolor)
{
	if (DiffuseSource == D3DMCS_COLOR1 && !AllowVertexColor)
	{
		return float4(1, 1, 1, vcolor.a);
	}

	if (DiffuseSource == D3DMCS_MATERIAL && ForceDefaultDiffuse)
	{
		return float4(178.0 / 255.0, 178.0 / 255.0, 178.0 / 255.0, MaterialDiffuse.a);
	}

	float4 color = (DiffuseSource == D3DMCS_COLOR1 && any(vcolor)) ? vcolor : MaterialDiffuse;

	int3 icolor = color.rgb * 255.0;
	if (icolor.r == 178 && icolor.g == 178 && icolor.b == 178)
	{
		return float4(1, 1, 1, color.a);
	}

	return color;
}


// Vertex shaders

PS_IN vs_main(VS_IN input)
{
	PS_IN output;

	output.position = mul(float4(input.position, 1), wvMatrix);
	output.fogDist  = output.position.z;
	output.position = mul(output.position, ProjectionMatrix);

#if defined(USE_TEXTURE) && defined(USE_ENVMAP)
	output.tex = (float2)mul(float4(input.normal, 1), wvMatrixInvT);
	output.tex = (float2)mul(float4(output.tex, 0, 1), TextureTransform);
#else
	output.tex = input.tex;
#endif

#if defined(USE_LIGHT)
	{
		float3 worldNormal = mul(input.normal * NormalScale, (float3x3)WorldMatrix);
		float4 diffuse = GetDiffuse(input.color);

		// This is the "brightness index" calculation. Just a dot product
		// of the vertex normal (in world space) and the light direction.
		float _dot = dot(LightDirection, worldNormal);

		// The palette's brightest point is 0, and its darkest point is 1,
		// so we push the dot product (-1 .. 1) into the rage 0 .. 1, and
		// subtract it from 1. This is the value we use for indexing into
		// the palette.
		// HACK: This clamp prevents a visual bug in the Mystic Ruins past (shrine on fire)
		float i = floor(clamp(1 - (_dot + 1) / 2, 0, 0.99) * 255) / 255;

		float4 pdiffuse;

		if (DiffuseOverride)
		{
			pdiffuse = float4(DiffuseOverrideColor, 1);
		}
		else
		{
			pdiffuse = tex2Dlod(atlasSamplerA, float4(i, DiffuseIndexA, 0, 0));
		}

		float4 pspecular = tex2Dlod(atlasSamplerA, float4(i, SpecularIndexA, 0, 0));

		#ifdef USE_BLEND
			float4 bdiffuse = tex2Dlod(atlasSamplerB, float4(i, DiffuseIndexB, 0, 0));
			float4 bspecular = tex2Dlod(atlasSamplerB, float4(i, SpecularIndexB, 0, 0));

			pdiffuse = lerp(pdiffuse, bdiffuse, DiffuseBlendFactor);
			pspecular = lerp(pspecular, bspecular, SpecularBlendFactor);
		#endif

		output.diffuse = float4((diffuse * pdiffuse).rgb, diffuse.a);
		output.specular = float4(pspecular.rgb, 0.0f);
	}
#else
	{
		// Just spit out the vertex or material color if lighting is off.
		output.diffuse = GetDiffuse(input.color);
		output.specular = 0;
	}
#endif

	return output;
}

float4 ps_main(PS_IN input) : COLOR
{
	float4 result;

#ifdef USE_TEXTURE
	result = tex2D(baseSampler, input.tex);
	result = result * input.diffuse + input.specular;
#else
	result = input.diffuse;
#endif

#ifdef USE_ALPHA
	clip(result.a < AlphaRef ? -1 : 1);
#endif

#ifdef USE_FOG
	float factor = CalcFogFactor(input.fogDist);
	result.rgb = (factor * result + (1.0 - factor) * FogColor).rgb;
#endif

	return result;
}
