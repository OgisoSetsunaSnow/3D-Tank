cbuffer PerFrameConstants : register(b0)
{
    float4x4 WorldViewProj;
}

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 worldDirection : TEXCOORD0;
};

VertexShaderOutput VS_main(
    float4 position : POSITION,
    float3 vtxcolor : TEXCOORD1,
    float2 uv : TEXCOORD0,
    float3 normal : NORMAL)
{
    VertexShaderOutput output;
    output.position = mul(float4(position.xyz, 1.0), WorldViewProj);
    // Z == W
    output.position = output.position.xyww;
    output.worldDirection = position.xyz;

    return output;
}

Texture2D<float4> skyboxTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main(float4 position : SV_POSITION, float3 worldDirection : TEXCOORD0) : SV_TARGET
{
#if D3D12_SAMPLE_TEXTURE
    float3 dir = normalize(worldDirection);
    float u = 0.5 + atan2(dir.z, dir.x) / (2.0 * 3.14159265359);
    float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) / 3.14159265359;

    return skyboxTexture.Sample(texureSampler, float2(u, v));
#else
    float3 dir = normalize(worldDirection);
    float t = saturate(dir.y * 0.5 + 0.5);
    float3 skyColor = lerp(float3(0.05, 0.05, 0.05), float3(0.4, 0.6, 0.9), t);
    return float4(skyColor, 1.0);
#endif
}
