float4x4 WorldMatrix      : register(c40);
float4x4 ViewMatrix       : register(c44);
float4x4 ProjectionMatrix : register(c48);

float4x4 l_WorldMatrix      : register(c60);
float4x4 l_ViewMatrix       : register(c64);
float4x4 l_ProjectionMatrix : register(c68);

//float2   Viewport         : register(c58);

void vs_main(in  float3 pos_in   : POSITION,
             out float4 pos_out  : POSITION,
             out float4 pos1     : TEXCOORD0,
             out float4 pos2     : TEXCOORD1)
{
	pos_out = mul(float4(pos_in, 1), WorldMatrix);
	pos_out = mul(pos_out, ViewMatrix);
	pos_out = mul(pos_out, ProjectionMatrix);

	pos1 = pos_out;

	pos2 = mul(float4(pos_in, 1), l_WorldMatrix);
	pos2 = mul(pos2, l_ViewMatrix);
	pos2 = mul(pos2, l_ProjectionMatrix);
}

float4 ps_main(float4 curr_pos : TEXCOORD0, float4 last_pos : TEXCOORD1) : COLOR
{
	float2 velocity = (curr_pos.xy / curr_pos.w) - (last_pos.xy / last_pos.w);

	velocity *= 0.5;
	velocity.y *= -1.0;

	return float4(velocity, 0, 0);
}
