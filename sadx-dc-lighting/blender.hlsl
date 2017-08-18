#define D3DBLEND_ZERO            1
#define D3DBLEND_ONE             2
#define D3DBLEND_SRCCOLOR        3
#define D3DBLEND_INVSRCCOLOR     4
#define D3DBLEND_SRCALPHA        5
#define D3DBLEND_INVSRCALPHA     6
#define D3DBLEND_DESTALPHA       7
#define D3DBLEND_INVDESTALPHA    8
#define D3DBLEND_DESTCOLOR       9
#define D3DBLEND_INVDESTCOLOR    10
#define D3DBLEND_SRCALPHASAT     11
#define EPSILON 1e-5

#define SAMPLER_DEFAULT \
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

float2 ViewPort : register(c46);
Texture2D BackBuffer : register(t1);
Texture2D AlphaLayer : register(t2);
Texture2D BlendLayer : register(t3);

SamplerState backBufferSampler : register(s1) = sampler_state
{
	Texture = BackBuffer;
	SAMPLER_DEFAULT;
};

SamplerState alphaLayerSampler : register(s2) = sampler_state
{
	Texture = AlphaLayer;
	SAMPLER_DEFAULT;
};

SamplerState blendLayerSampler : register(s3) = sampler_state
{
	Texture = BlendLayer;
	SAMPLER_DEFAULT;
};

float4 get_blend_factor(in float mode, in float4 source, in float4 destination)
{
	switch (max(0, floor(mode)))
	{
		default: // error state
			return float4(1, 0, 0, 1);
		case D3DBLEND_ZERO:
			return float4(0, 0, 0, 0);
		case D3DBLEND_ONE:
			return float4(1, 1, 1, 1);
		case D3DBLEND_SRCCOLOR:
			return source;
		case D3DBLEND_INVSRCCOLOR:
			return 1.0f - source;
		case D3DBLEND_SRCALPHA:
			return source.aaaa;
		case D3DBLEND_INVSRCALPHA:
			return 1.0f - source.aaaa;
		case D3DBLEND_DESTALPHA:
			return destination.aaaa;
		case D3DBLEND_INVDESTALPHA:
			return 1.0f - destination.aaaa;
		case D3DBLEND_DESTCOLOR:
			return destination;
		case D3DBLEND_INVDESTCOLOR:
			return 1.0f - destination;
		case D3DBLEND_SRCALPHASAT:
			float f = min(source.a, 1 - destination.a);
			return float4(f, f, f, 1);
	}
}

float4 blend_colors(in float srcBlend, in float dstBlend, float4 texel, float4 pixel)
{
	float4 result;

	float4 src = get_blend_factor(srcBlend, texel, pixel);
	float4 dst = get_blend_factor(dstBlend, texel, pixel);
	return (texel * src) + (pixel * dst);
}

#define THING(v) \
if (n - (float)v <= EPSILON) \
	return v

float4 vs_main(in float3 pos : POSITION) : POSITION
{
	return float4(pos, 1);
}

float4 ps_main(float2 vpos : VPOS) : COLOR
{
	float2 coord     = vpos / ViewPort;
	float4 backcolor = tex2D(backBufferSampler, coord);
	float4 layer     = tex2D(alphaLayerSampler, coord);
	float4 blend     = tex2D(blendLayerSampler, coord);

	if (!any(blend))
	{
		return backcolor;
	}

	float source = blend.r * 255;
	float destination = blend.g * 255;

	return float4(blend_colors(source, destination, layer, backcolor).rgb, 1);
}
