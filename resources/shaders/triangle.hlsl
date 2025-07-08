
cbuffer Global : register(b0, space1)
{
    float3x3 Projection : packoffset(c0);
};

struct Input
{
    float2 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct Output
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_Position;
};

Output vertex(Input input)
{
    Output output;
    output.TexCoord = input.TexCoord;

    float3 presentPosition = mul(float3(input.Position, 1.0f), Projection);

    output.Position = float4(presentPosition.xy, 0.0f, 1.0f);    
    return output;
}

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

float4 fragment(Output input) : SV_Target0
{ 
    return Texture.Sample(Sampler, input.TexCoord);
}