#include <metal_stdlib>
using namespace metal;

// ---------------------------------------------------------------------------
// Shared data structures
// ---------------------------------------------------------------------------
struct MeshVertexIn {
    float3 position [[attribute(0)]];
    float3 vtxcolor [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    float3 normal   [[attribute(3)]];
};

struct VertexOut_Basic {
    float4 position [[position]];
    float2 uv;
    float4 color;
    float3 worldPos;
    float3 normal;
};

struct VertexOut_Instanced {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

struct ConstantBuffer {
    float4x4 worldViewProj;
    float4x4 world;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
};

struct PerInstanceData3D {
    float4x4 xform;
    float4   uv_offset_scale;
    float4   color;
};

struct PerInstanceData2D {
    float4x4 xform;
    float4   uv_offset_scale;
    float4   color;
};

// ---------------------------------------------------------------------------
// Opaque Basic – non-instanced mesh draw
//   Binding: buffer(0)=vertex(stage_in), buffer(1)=cb, texture(0)
// ---------------------------------------------------------------------------
vertex VertexOut_Basic vs_opaque_basic(
    MeshVertexIn         in [[stage_in]],
    constant ConstantBuffer& cb [[buffer(1)]])
{
    VertexOut_Basic out;
    out.position = cb.worldViewProj * float4(in.position, 1.0);
    out.uv       = in.uv;
    out.color = float4(in.vtxcolor, 1.0);
    out.worldPos = (cb.world * float4(in.position, 1.0)).xyz;
    out.normal = (cb.world * float4(in.normal, 0.0)).xyz;
    return out;
}

fragment float4 fs_opaque_basic(
    VertexOut_Basic     in  [[stage_in]],
    constant ConstantBuffer& cb [[buffer(1)]],
    texture2d<float>   tex  [[texture(0)]],
    sampler            smp  [[sampler(0)]])
{
    float3 N = normalize(in.normal);
    float3 L = normalize(-cb.lightDir.xyz);
    float3 V = normalize(cb.cameraPos.xyz - in.worldPos);
    float3 H = normalize(L + V);

    float4 texColor = tex.sample(smp, in.uv);
    float3 albedo = texColor.rgb * in.color.rgb;

    // ambient
    float3 ambient = 0.15 * albedo;

    // diffuse
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = NdotL * cb.lightColor.rgb * albedo;

    // specular (Blinn-Phong)
    float roughness = 0.4;
    float specularStrength = 0.35;
    float shininess = mix(64.0, 4.0, roughness);   // MSL 用 mix 代替 HLSL 的 lerp
    float NdotH = max(dot(N, H), 0.0);
    float specFactor = pow(NdotH, shininess);
    float3 specular = specFactor * cb.lightColor.rgb * specularStrength * (1.0 - roughness);

    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, texColor.a);
}

// ---------------------------------------------------------------------------
// Instance 3D – instanced world-space draws (quads, spheres)
//   Binding: buffer(0)=vertex(stage_in), buffer(1)=cb,
//            buffer(2)=PerInstanceData3D[], texture(0)
// ---------------------------------------------------------------------------
vertex VertexOut_Instanced vs_instance_3d(
    MeshVertexIn                          in          [[stage_in]],
    constant ConstantBuffer&              cb          [[buffer(1)]],
    const device PerInstanceData3D*       instances   [[buffer(2)]],
    uint                                  instanceId  [[instance_id]])
{
    VertexOut_Instanced out;
    float4x4 xform = instances[instanceId].xform;
    out.position  = cb.worldViewProj * (xform * float4(in.position, 1.0));
    float4 uvOS   = instances[instanceId].uv_offset_scale;
    out.uv        = in.uv * uvOS.zw + uvOS.xy;
    out.color     = instances[instanceId].color;
    return out;
}

// ---------------------------------------------------------------------------
// Instance 2D – instanced clip-space quads
//   Binding: buffer(2)=PerInstanceData2D[], texture(0)
//   vertex_id comes from the quad index buffer → values in {0,1,2,3}
// ---------------------------------------------------------------------------
vertex VertexOut_Instanced vs_instance_2d(
    uint                                  vertexId    [[vertex_id]],
    uint                                  instanceId  [[instance_id]],
    const device PerInstanceData2D*       instances   [[buffer(2)]])
{
    const float hw = 0.5;
    float4 corners[4] = {
        float4(-hw, -hw, 0, 1),
        float4( hw, -hw, 0, 1),
        float4(-hw,  hw, 0, 1),
        float4( hw,  hw, 0, 1)
    };
    float2 uvs[4] = {
        float2(0, 1), float2(1, 1),
        float2(0, 0), float2(1, 0)
    };

    uint idx       = (vertexId < 4u) ? vertexId : 0u;
    float4 clipPos = corners[idx];
    float2 uv      = uvs[idx];

    VertexOut_Instanced out;
    out.position = instances[instanceId].xform * clipPos;
    float4 uvOS  = instances[instanceId].uv_offset_scale;
    out.uv       = uv * uvOS.zw + uvOS.xy;
    out.color    = instances[instanceId].color;
    return out;
}

// Shared fragment shader for both instanced variants
fragment float4 fs_instanced(
    VertexOut_Instanced  in  [[stage_in]],
    texture2d<float>    tex  [[texture(0)]],
    sampler             smp  [[sampler(0)]])
{
    float4 c = tex.sample(smp, in.uv) * in.color;
    if (c.a < 0.01) discard_fragment();
    return c;
}

// ---------------------------------------------------------------------------
// Skybox – renders a cubemap-like texture on an inverted box
//   Binding: buffer(0)=vertex(stage_in), buffer(1)=cb, texture(0)
//   Uses the xyww trick: output.position.xyww makes z/w = 1.0 after
//   perspective divide, always placing the skybox at max depth.
// ---------------------------------------------------------------------------
struct VertexOut_Skybox {
    float4 position [[position]];
    float3 worldDirection;
};

vertex VertexOut_Skybox vs_skybox(
    MeshVertexIn         in [[stage_in]],
    constant ConstantBuffer& cb [[buffer(1)]])
{
    VertexOut_Skybox out;
    out.position = cb.worldViewProj * float4(in.position, 1.0);
    // Z == W trick: makes z/w = 1.0 after perspective divide (max depth)
    out.position = out.position.xyww;
    out.worldDirection = in.position;
    return out;
}

fragment float4 fs_skybox(
    VertexOut_Skybox     in  [[stage_in]],
    texture2d<float>    tex  [[texture(0)]],
    sampler             smp  [[sampler(0)]])
{
    float3 dir = normalize(in.worldDirection);
    float u = 0.5 + atan2(dir.z, dir.x) / (2.0 * M_PI_F);
    float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) / M_PI_F;
    return tex.sample(smp, float2(u, v));
}
