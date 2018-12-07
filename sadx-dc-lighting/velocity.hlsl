float4x4 WorldMatrix      : register(c0);
float4x4 wvMatrix         : register(c4);
float4x4 ProjectionMatrix : register(c8);

float4x4 CurrentTransform : register(c50);
float4x4 LastTransform    : register(c54);
//float2   Viewport         : register(c58);

void vs_main(in  float4 pos_in : POSITION,
             out float4 pos_out  : POSITION,
             out float4 pos1 : TEXCOORD0,
             out float4 pos2 : TEXCOORD1)
{
	pos_out = mul(pos_in, wvMatrix);
	pos_out = mul(pos_out, ProjectionMatrix);

	pos1 = pos_out;
	pos2 = mul(pos_in, LastTransform);
}

float4 ps_main(float2 bullshit : VPOS, float4 curr_pos : TEXCOORD0, float4 last_pos : TEXCOORD1) : COLOR
{
	float2 velocity = (curr_pos.xy / curr_pos.w) - (last_pos.xy / last_pos.w);

	velocity *= 0.5;
	velocity.y *= -1.0;

	return float4(velocity, 1.0f, 1.0f);
}
