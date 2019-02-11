float4x4 WorldMatrix      : register(c40);
float4x4 ViewMatrix       : register(c44);
float4x4 ProjectionMatrix : register(c48);

void vs_main(in float3 pos_in : POSITION, out float4 pos_out : POSITION)
{
	pos_out = mul(float4(pos_in, 1), WorldMatrix);
	pos_out = mul(pos_out, ViewMatrix);
	pos_out = mul(pos_out, ProjectionMatrix);
}

float4 ps_main() : COLOR
{
	return float4(0.5, 0.5, 0.0, 1.0);
}
