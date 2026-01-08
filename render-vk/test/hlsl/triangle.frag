
Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
  float4 color = textureColor.Sample(samplerColor, input.UV);

  return color;
}
