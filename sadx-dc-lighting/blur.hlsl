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

#define MAX_SAMPLES 16

// d3d9 fucking sucks god fuckgin dasrjhfbaw4lktjh423fj,hwdv
#define g_vSourceDimensions float2(1280, 720);

float4 MotionBlur(float2 vTexCoord, float2 vPixelVelocity, int iNumSamples)
{
	// Clamp to a max velocity.  The max we can go without artifacts os
	// is 1.4f * iNumSamples...but we can fudge things a little.
	float2 maxVelocity = (2.0f * iNumSamples) / g_vSourceDimensions;
	vPixelVelocity = clamp(vPixelVelocity, -maxVelocity, maxVelocity);

	float2 vFinalSamplePos = vTexCoord + vPixelVelocity;

	// For each sample, sum up each sample's color in "vSum" and then divide
	// to average the color after all the samples are added.
	float4 vSum = 0;
	for (int i = 0; i < iNumSamples; i++)
	{
		// Sample texture in a new spot based on vPixelVelocity vector 
		// and average it with the other samples
		float2 vSampleCoord = vTexCoord + (vPixelVelocity * (i / (float)iNumSamples));

		// Lookup the color at this new spot
		float4 vSample = tex2D(backbuffer, vSampleCoord);

		// Add it with the other samples
		vSum += vSample;
	}

	// Return the average color of all the samples
	return float4((vSum / (float)iNumSamples).rgb, 1);
}

float4 ps_main(in float2 uv_in : TEXCOORD) : COLOR
{
	float2 v = tex2D(velocitybuff, uv_in).rg;
	return MotionBlur(uv_in, v, MAX_SAMPLES);
}
