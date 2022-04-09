Texture2D backbuffer_tex   : register(t1);
Texture2D velocitybuff_tex : register(t2);

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

float4 blur(in float2 coordinates, in float2 velocity, in int sample_count)
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
	float2 pixel_size = float2(1 / Viewport.x, 1 / Viewport.y);

	// Sample in a 3x3 grid, with the current pixel being dead center.
	float4 grid[9];

	grid[0] = tex2D(velocitybuff, uv_in + float2(-pixel_size.x, -pixel_size.y)); // Top left
	grid[1] = tex2D(velocitybuff, uv_in + float2(            0, -pixel_size.y)); // Top center
	grid[2] = tex2D(velocitybuff, uv_in + float2( pixel_size.x, -pixel_size.y)); // Top right
	grid[3] = tex2D(velocitybuff, uv_in + float2(-pixel_size.x,             0)); // Middle left
	grid[4] = tex2D(velocitybuff, uv_in);                                        // Middle (center) - no offset
	grid[5] = tex2D(velocitybuff, uv_in + float2( pixel_size.x,             0)); // Middle right
	grid[6] = tex2D(velocitybuff, uv_in + float2(-pixel_size.x,  pixel_size.y)); // Bottom left
	grid[7] = tex2D(velocitybuff, uv_in + float2(            0,  pixel_size.y)); // Bottom center
	grid[8] = tex2D(velocitybuff, uv_in + float2( pixel_size.x,  pixel_size.y)); // Bottom right

	#define AVERAGE

	float2 velocity = 0;

	#if !defined(AVERAGE)
		float length_sq = 0;
	#endif

	for (int i = 0; i < 9; i++)
	{
		#if defined(AVERAGE)
			velocity += grid[i].xy;
		#else
			float2 v = (grid[i].xy * 2.0) - 1.0;
			float vl = v.x * v.x + v.y * v.y;

			if (vl > length_sq)
			{
				velocity = v;
				length_sq = vl;
			}
		#endif
	}

	#if defined(AVERAGE)
		velocity /= 9.0;
		velocity = (velocity * 2.0) - 1.0;
	#endif

	velocity.x = -velocity.x;

	if ((int)(velocity.x * 127) == 0 && (int)(velocity.y * 127) == 0)
	{
		return tex2D(backbuffer, uv_in);
	}

	const float speed = length(velocity * Viewport);
	const int sample_count = clamp((int)speed, 1, min(MAX_SAMPLES, min(Viewport.x, Viewport.y)));

	return blur(uv_in, velocity, sample_count);
}
