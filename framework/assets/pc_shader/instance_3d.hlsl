cbuffer PerFrameConstants : register (b0)
{
	float4x4 WorldViewProj;
}

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
};

struct PerInstanceData3D
{
	float4x4 xform;
	float4 uv_offset_scale;
	float4 color;
};

struct PerInstanceData2D
{
	float4 clip_offset_scale;
	float4 uv_offset_scale;
	float4 color;
};

StructuredBuffer<PerInstanceData3D> instanceData3D : register(t4);
StructuredBuffer<PerInstanceData2D> instanceData2D : register(t5);

VertexShaderOutput VS_main(
	float4 position : POSITION,
	float4 vtxcolor : TEXCOORD1,
	float2 uv : TEXCOORD0,
	uint instanceId : SV_INSTANCEID)
{
	VertexShaderOutput output;

	output.position = mul(mul(position, instanceData3D[instanceId].xform), WorldViewProj);
	
	float4 uv_offset_scale = instanceData3D[instanceId].uv_offset_scale;
	output.uv = uv * uv_offset_scale.zw + uv_offset_scale.xy;
	output.color = instanceData3D[instanceId].color;

	return output;
}

Texture2D<float4> baseTexture : register(t0);
SamplerState texureSampler    : register(s0);

float4 PS_main (float4 position : SV_POSITION, float2 uv : TEXCOORD0, float4 color : TEXCOORD1) : SV_TARGET
{
#if D3D12_SAMPLE_TEXTURE
	float4 finalcolor = baseTexture.Sample (texureSampler, uv) * color;
	if(finalcolor.w < 0.01f)
		discard;
	return finalcolor;
#else
	return color;
#endif
}

