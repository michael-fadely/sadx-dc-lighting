Texture2D backbuffer_tex   : register(t1);
Texture2D velocitybuff_tex : register(t2);

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

SamplerState backbuffer : register(s1) = sampler_state
{
	Texture = backbuffer_tex;
	SAMPLER_DEFAULT;
};

SamplerState velocitybuff : register(s2) = sampler_state
{
	Texture = velocitybuff_tex;
	SAMPLER_DEFAULT;
};

#define MAX_SAMPLES 64
#define BLUR_AMOUNT 0.5

float2 Viewport : register(c52);

float4 blur(float2 coordinates, float2 velocity, int sample_count)
{
	const float4 coord = float4(coordinates.xy, 0, 1);
	float4 result = tex2D(backbuffer, coordinates);

	if (sample_count == 1)
	{
		return result;
	}

	for (int i = 1; i < sample_count; ++i)
	{
		float2 offset = velocity * ((float)i / (float)(sample_count - 1) - 0.5);
		result += tex2Dlod(backbuffer, coord + float4(offset * BLUR_AMOUNT, 0, 0));
	}

	result /= sample_count;

	return float4(result.rgb, 1);
}

float4 ps_main(in float2 uv_in : TEXCOORD) : COLOR
{
	float4 texels = tex2D(velocitybuff, uv_in);

	if ((int)(texels.r * 255) == 127 && (int)(texels.g * 255) == 127)
	{
		return tex2D(backbuffer, uv_in);
	}

	float2 velocity = ((texels.rg * 2.0) - 1.0);
	velocity.x = -velocity.x;

	float speed = length(velocity * Viewport);
	int sample_count = clamp((int)speed, 1, min(MAX_SAMPLES, min(Viewport.x, Viewport.y)));

	return blur(uv_in, velocity, sample_count);
}
