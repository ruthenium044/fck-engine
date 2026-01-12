struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float4 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
};
cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
	[[vk::location(0)]] float4 Color : COLOR0;
	[[vk::location(1)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

    output.Pos = mul(ubo.projection, float4(input.Pos.xy, 0.0f, 1.0));
    // Might need to flip pos. WE will see!
    
	output.Color = input.Color;
	output.UV = input.UV;
	return output;
}
