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

float2 ViewPort;
Texture2D BackBuffer;
Texture2D AlphaLayer;
Texture2D BlendLayer;

sampler backBufferSampler = sampler_state
{
	Texture   = <BackBuffer>;
	MinFilter = Point;
	MagFilter = Point;
	AddressU  = Clamp;
	AddressV  = Clamp;
};
sampler alphaLayerSampler = sampler_state
{
	Texture   = <AlphaLayer>;
	MinFilter = Point;
	MagFilter = Point;
	AddressU  = Clamp;
	AddressV  = Clamp;
};
sampler blendLayerSampler = sampler_state
{
	Texture   = <BlendLayer>;
	MinFilter = Point;
	MagFilter = Point;
	AddressU  = Clamp;
	AddressV  = Clamp;
};


float4 GetBlend(uint mode, float4 source, float4 destination)
{
	switch (mode)
	{
		default:
		case D3DBLEND_ZERO:
			return 0.0f;
		case D3DBLEND_ONE:
			return 1.0f;
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

float4 Blend(uint srcBlend, uint dstBlend, float4 texel, float4 pixel)
{
	float4 result;
	float4 src = GetBlend(srcBlend, texel, pixel);
	float4 dst = GetBlend(dstBlend, texel, pixel);
	return (texel * src) + (pixel * dst);
}


#define THING(v) \
if (n - (float)v <= EPSILON) \
	return v

uint garbage(float n, uint _default)
{
	THING(D3DBLEND_ZERO);
	THING(D3DBLEND_ONE);
	THING(D3DBLEND_SRCCOLOR);
	THING(D3DBLEND_INVSRCCOLOR);
	THING(D3DBLEND_SRCALPHA);
	THING(D3DBLEND_INVSRCALPHA);
	THING(D3DBLEND_DESTALPHA);
	THING(D3DBLEND_INVDESTALPHA);
	THING(D3DBLEND_DESTCOLOR);
	THING(D3DBLEND_INVDESTCOLOR);
	THING(D3DBLEND_SRCALPHASAT);

	return _default;
}

float4 vs_blender(in float3 pos : POSITION) : POSITION
{
	return float4(pos, 1);
}

float4 ps_blender(float2 vpos : VPOS) : COLOR
{
	float4 result;

	float2 coord     = vpos / ViewPort;
	float4 backcolor = tex2D(backBufferSampler, coord);
	float4 layer     = tex2D(alphaLayerSampler, coord);
	float4 blend     = tex2D(blendLayerSampler, coord);

	if (!any(blend))
	{
		return backcolor;
	}

	float src = round(blend.r * 15.0f);
	float dst = round(blend.g * 15.0f);

	uint source = garbage(src, D3DBLEND_SRCALPHA);
	uint destination = garbage(dst, D3DBLEND_INVSRCALPHA);

	result = Blend(source, destination, layer, backcolor);
	return result;
}

technique Main
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_blender();
		PixelShader = compile ps_3_0 ps_blender();
	}
}
