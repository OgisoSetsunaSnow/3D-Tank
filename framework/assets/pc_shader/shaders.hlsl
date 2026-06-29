cbuffer PerFrameConstants : register (b0)
{
	float4x4 WorldViewProj;
}

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VertexShaderOutput VS_main(
	float4 position : POSITION,
	float4 vtxcolor : TEXCOORD1,
	float2 uv : TEXCOORD0)
{
	VertexShaderOutput output;

	output.position = mul(position, WorldViewProj);
	output.uv = uv;

	return output;
}

Texture2D<float4> baseTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main (float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET
{
#if D3D12_SAMPLE_TEXTURE
	return baseTexture.Sample (texureSampler, uv);
#else
	return float4(1.0, 1.0, 1.0, 1.0);
#endif
}

