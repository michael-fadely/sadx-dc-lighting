float4x4 current_transform : c0;
float4x4 last_transform    : c4;

struct VS_IN
{
	float3 position : POSITION;
};

void vs_main(VS_IN input,
             out float4 pos  : POSITION,
             out float4 pos2 : TEXCOORD0)
{
	pos  = mul(float4(input.position, 1), current_transform);
	pos2 = mul(float4(input.position, 1), last_transform);
}

float4 ps_main(float4 curr_pos : POSITION1, float4 last_pos : TEXCOORD0) : COLOR
{
	float2 a = curr_pos.xy / curr_pos.w;
	a = a * 0.5 + 0.5;

	float2 b = last_pos.xy / last_pos.w;
	b = b * 0.5 + 0.5;

	return float4(a - b, 0, 0);
}
