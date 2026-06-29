cbuffer PerFrameConstants : register (b0)
{
	float4x4 WorldViewProj;
	float4x4 World;
	float4 CameraPos;
	float4 LightDir;
	float4 LightColor;
}

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float4 color : TEXCOORD1;
	float3 worldPos : TEXCOORD2;
	float3 normal : NORMAL;
};


VertexShaderOutput VS_main(
	float4 position : POSITION,
	float3 vtxcolor : TEXCOORD1,
	float2 uv : TEXCOORD0,
	float3 normal : NORMAL)
{
    VertexShaderOutput output;

    output.position = mul(position, WorldViewProj);
    output.uv       = uv;
    output.color    = float4(vtxcolor, 1.0f);

    output.worldPos = mul(position, World).xyz;
    output.normal = mul(float4(normal, 0.0f), World).xyz;

    return output;
}

Texture2D<float4> baseTexture : register(t0);
SamplerState texureSampler      : register(s0);

float4 PS_main (float4 position : SV_POSITION, float2 uv : TEXCOORD0, float4 color : TEXCOORD1, float3 worldPos : TEXCOORD2, float3 normal : NORMAL) : SV_TARGET
{
    float3 N = normalize(normal);
    float3 L = normalize(-LightDir.xyz);
    float3 V = normalize(CameraPos.xyz - worldPos);
    float3 H = normalize(L + V);

    float4 texColor = baseTexture.Sample(texureSampler, uv);

#if D3D12_SAMPLE_TEXTURE
    if (texColor.a < 0.01f)
        discard;

    float3 albedo = texColor.rgb * color.rgb;
#else
    float3 albedo = color.rgb;
#endif
    float3 ambient = 0.15f * albedo;

    float  NdotL   = max(dot(N, L), 0.0f);
    float3 diffuse = NdotL * LightColor.rgb * albedo;

	float roughness = 0.4f;
	float specularStrength = 0.35f;
	
	float shininess = lerp(64.0f, 4.0f, roughness);

    float  NdotH    = max(dot(N, H), 0.0f);
    float  specFactor = pow(NdotH, shininess);
	float3 specular = specFactor * LightColor.rgb * specularStrength * (1.0f - roughness);

    float3 finalColor = ambient + diffuse + specular;

#if D3D12_SAMPLE_TEXTURE
    return float4(finalColor, texColor.a);
#else
    return float4(finalColor, 1.0f);
#endif
}

