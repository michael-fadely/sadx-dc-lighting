Texture2D tex : register(t1);

// FIXME: copy/pasted
#define SAMPLER_DEFAULT     \
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

SamplerState tex_sampler : register(s1) = sampler_state
{
	Texture = tex;
	SAMPLER_DEFAULT;
};

void vs_main(in  float3 pos_in  : POSITION,
             out float4 pos_out : POSITION,
             in  float2 uv_in   : TEXCOORD,
             out float2 uv_out  : TEXCOORD)
{
	uv_out = uv_in;
	pos_out = float4(pos_in, 1);
}

float4 ps_main(in float2 uv : TEXCOORD) : COLOR
{
	return tex2D(tex_sampler, uv);
}
