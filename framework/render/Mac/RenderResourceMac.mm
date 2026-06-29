#include <Platform.h>

#if VTS_PLATFORM_OSX
#import <Metal/Metal.h>
#include "render/Mac/RenderMac.h"
#include "render/Mac/WindowMac.h"
#include <assert.h>
#include <string.h>

// ---------------------------------------------------------------------------
// RenderBufferUtil::CopyShaderMatrix – copy src into dst for Metal upload.
//
// Why no transpose here (unlike the D3D12/PC version):
//  - The C++ float[4][4] matrices are stored row-major with translations in
//    row 3, designed for HLSL's "mul(v, M)" row-vector convention.
//  - Metal's float4x4 is column-major.  When Metal reads the same raw bytes,
//    it effectively sees the transpose of the C++ matrix.  Our MSL shaders
//    use "M * v" (column-vector), so:
//      M_metal * v  =  M_cpp^T * v  =  same values as "mul(v, M_cpp)" in HLSL.
//  - Therefore: just copying the bytes gives the correct transform in Metal.
//    Transposing before upload (as the PC side does) would double-transpose and
//    land the translation in the w-row, corrupting every homogeneous divide.
// ---------------------------------------------------------------------------
namespace RenderBufferUtil
{
    void CopyShaderMatrix(float(&dst)[4][4], float(&src)[4][4])
    {
        memcpy(dst, src, sizeof(dst));
    }
}

// ---------------------------------------------------------------------------
// RenderMeshResourceMac
// ---------------------------------------------------------------------------
void RenderMeshResourceMac::UploadToGPU(RenderMac* rnd)
{
    if (!gpuDirty && vertexBuffer && indexBuffer)
        return;

    // Vertex buffer
    uint32_t newVtxBytes = VerticesCPU.GetDataBytes();
    if (newVtxBytes > 0 && (vertexBytes < newVtxBytes || !vertexBuffer))
    {
        vertexBuffer = [rnd->device_ newBufferWithBytes:VerticesCPU.Data
                                                 length:newVtxBytes
                                                options:MTLResourceStorageModeShared];
        vertexBytes = newVtxBytes;
    }
    else if (vertexBuffer && IsDynamicMesh && newVtxBytes > 0)
    {
        // Dynamic mesh: update in-place (StorageModeShared allows CPU writes)
        memcpy(vertexBuffer.contents, VerticesCPU.Data, newVtxBytes);
    }

    // Index buffer
    uint32_t newIdxBytes = IndicesCPU.GetDataBytes();
    if (newIdxBytes > 0 && (indexBytes < newIdxBytes || !indexBuffer))
    {
        indexBuffer = [rnd->device_ newBufferWithBytes:IndicesCPU.Data
                                                length:newIdxBytes
                                               options:MTLResourceStorageModeShared];
        indexBytes = newIdxBytes;
    }
    else if (indexBuffer && IsDynamicMesh && newIdxBytes > 0)
    {
        memcpy(indexBuffer.contents, IndicesCPU.Data, newIdxBytes);
    }

    gpuDirty = false;
}

// ---------------------------------------------------------------------------
// RenderTextureResourceMac
// ---------------------------------------------------------------------------
void RenderTextureResourceMac::UploadToGPU(RenderMac* rnd)
{
    if (texture)
        return;  // already on GPU

    MTLTextureDescriptor* desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                           width:(NSUInteger)Width
                                                          height:(NSUInteger)Height
                                                       mipmapped:NO];
    desc.usage       = MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModeShared;  // Apple Silicon: unified memory — no CPU/GPU sync needed

    texture = [rnd->device_ newTextureWithDescriptor:desc];

    MTLRegion region = MTLRegionMake2D(0, 0, (NSUInteger)Width, (NSUInteger)Height);
    [texture replaceRegion:region
               mipmapLevel:0
                 withBytes:RGBATextureDataCPU
               bytesPerRow:(NSUInteger)(Width * RGBAPixel::GetBytesPerPixel())];
}

// ---------------------------------------------------------------------------
// RenderResourceMac – PrepareRender / Render / ExitRender
// ---------------------------------------------------------------------------
void RenderResourceMac::PrepareRender(RenderInterface* rnd)
{
    RenderMac* r = static_cast<RenderMac*>(rnd);

    auto* mesh = static_cast<RenderMeshResourceMac*>(MeshResource);
    auto* tex  = static_cast<RenderTextureResourceMac*>(TextureResource);

    assert(mesh && tex);
    mesh->UploadToGPU(r);
    tex->UploadToGPU(r);
}

void RenderResourceMac::DrawIndexed(RenderMac* rnd, id<MTLRenderCommandEncoder> enc)
{
    auto* mesh = static_cast<RenderMeshResourceMac*>(MeshResource);
    auto* tex  = static_cast<RenderTextureResourceMac*>(TextureResource);
    assert(mesh && tex && ShaderResource);

    // Bind PSO
    bool alphaBlend = EnableAlphaBlend;
    bool depthTest  = EnableDepthTest;
    auto* pso = rnd->GetOrCreatePSO(alphaBlend, depthTest, ShaderResource->ShaderType);
    [enc setRenderPipelineState:pso];
    MTLCompareFunction depthCmp = (ShaderResource->ShaderType == RenderShaderResource::ShaderType_Skybox)
    ? MTLCompareFunctionLessEqual
    : MTLCompareFunctionLess;
    [enc setDepthStencilState:rnd->GetOrCreateDepthState(depthTest, EnableDepthWrite, depthCmp)];

    // Winding order
    [enc setFrontFacingWinding:
        (MeshResource->CullMode == RenderMeshResource::CCW_FRONT)
            ? MTLWindingCounterClockwise
            : MTLWindingClockwise];
    [enc setCullMode:EnableBackFaceCull ? MTLCullModeBack : MTLCullModeNone];
    [enc setTriangleFillMode:IsWireFrameMode ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    // Vertex buffer (stage_in at index 0)
    [enc setVertexBuffer:mesh->vertexBuffer offset:0 atIndex:0];

    // Constant buffer (at index 1)
    ConstantBufferMac cb;
    memcpy(&cb, &ConstBufferCPU, sizeof(ConstantBufferMac));
    [enc setVertexBytes:&cb length:sizeof(cb) atIndex:1];
    [enc setFragmentBytes:&cb length:sizeof(cb) atIndex:1];

    // Texture + sampler
    [enc setFragmentTexture:tex->texture atIndex:0];
    [enc setFragmentSamplerState:rnd->linearSampler_ atIndex:0];

    // Draw
    MTLPrimitiveType primType = (PrimitiveType == RenderResource::Primitive_LineList)
                                    ? MTLPrimitiveTypeLine
                                    : MTLPrimitiveTypeTriangle;
    [enc drawIndexedPrimitives:primType
                    indexCount:(NSUInteger)MeshResource->IndicesCPU.NumElements
                     indexType:MTLIndexTypeUInt32
                   indexBuffer:mesh->indexBuffer
             indexBufferOffset:0];
}

void RenderResourceMac::Render(RenderInterface* rnd)
{
    PrepareRender(rnd);  // idempotent
    RenderMac* r = static_cast<RenderMac*>(rnd);
    DrawIndexed(r, r->currentEncoder_);
}

void RenderResourceMac::ExitRender(RenderInterface* rnd, bool waitForRenderDone)
{
    (void)rnd;
    if (waitForRenderDone)
    {
        // Drain all in-flight GPU work by submitting an empty command buffer and waiting.
        RenderMac* r = static_cast<RenderMac*>(rnd);
        if (r && r->commandQueue_)
        {
            id<MTLCommandBuffer> sync = [r->commandQueue_ commandBuffer];
            [sync commit];
            [sync waitUntilCompleted];
        }
    }
}

// ---------------------------------------------------------------------------
// RenderResourceInstancedMac – instanced draws
// ---------------------------------------------------------------------------
void RenderResourceInstancedMac::DrawInstanced(RenderMac* rnd, id<MTLRenderCommandEncoder> enc)
{
    auto* mesh = static_cast<RenderMeshResourceMac*>(MeshResource);
    auto* tex  = static_cast<RenderTextureResourceMac*>(TextureResource);
    assert(mesh && tex && ShaderResource);

    auto shaderType = ShaderResource->ShaderType;
    bool is2D = (shaderType == RenderShaderResource::ShaderType_Instance2D);

    // Determine which CPU buffer to use and bail early if empty
    uint32_t   neededBytes = is2D ? m_InstancedData2D.DataBytes()       : m_InstancedData3D.DataBytes();
    NSUInteger instCount   = is2D ? (NSUInteger)m_InstancedData2D.NumElementsCPU
                                  : (NSUInteger)m_InstancedData3D.NumElementsCPU;
    if (instCount == 0)
        return;

    // Get / grow the persistent GPU buffer for this triple-buffer slot
    int           slot     = rnd->currentSlot_;
    id<MTLBuffer>__strong* pBuf     = is2D ? &instGpuBuf2D_[slot]         : &instGpuBuf3D_[slot];
    uint32_t*      pCap     = is2D ? &instGpuBuf2DCapacity_[slot] : &instGpuBuf3DCapacity_[slot];

    if (*pBuf == nil || *pCap < neededBytes)
    {
        *pBuf = [rnd->device_ newBufferWithLength:neededBytes
                                          options:MTLResourceStorageModeShared];
        *pCap = neededBytes;
    }

    // Write CPU data directly into the shared buffer (zero-copy on Apple Silicon)
    void* src = is2D ? (void*)m_InstancedData2D.DataCPU : (void*)m_InstancedData3D.DataCPU;
    memcpy((*pBuf).contents, src, neededBytes);

    id<MTLBuffer> instBuf = *pBuf;
    if (!instBuf)
        return;

    // PSO + depth state
    auto* pso = rnd->GetOrCreatePSO(EnableAlphaBlend, EnableDepthTest, shaderType);
    [enc setRenderPipelineState:pso];
    [enc setDepthStencilState:rnd->GetOrCreateDepthState(EnableDepthTest, EnableDepthWrite)];
    [enc setFrontFacingWinding:MTLWindingCounterClockwise];
    [enc setCullMode:MTLCullModeNone];  // quads are double-sided
    [enc setTriangleFillMode:IsWireFrameMode ? MTLTriangleFillModeLines : MTLTriangleFillModeFill]; 
    // Instance buffer at slot 2
    [enc setVertexBuffer:instBuf offset:0 atIndex:2];

    // Texture + sampler
    [enc setFragmentTexture:tex->texture atIndex:0];
    [enc setFragmentSamplerState:rnd->linearSampler_ atIndex:0];

    MTLPrimitiveType primType = (PrimitiveType == RenderResource::Primitive_LineList)
                                    ? MTLPrimitiveTypeLine
                                    : MTLPrimitiveTypeTriangle;

    if (!is2D)
    {
        // Instance3D: use stage_in (vertex buffer at slot 0) + constant buffer at slot 1
        [enc setVertexBuffer:mesh->vertexBuffer offset:0 atIndex:0];
        ConstantBufferMac cb;
        memcpy(&cb, &ConstBufferCPU, sizeof(cb));
        [enc setVertexBytes:&cb length:sizeof(cb) atIndex:1];
        [enc setFragmentBytes:&cb length:sizeof(cb) atIndex:1];

        [enc drawIndexedPrimitives:primType
                        indexCount:(NSUInteger)MeshResource->IndicesCPU.NumElements
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:mesh->indexBuffer
                 indexBufferOffset:0
                     instanceCount:instCount];
    }
    else
    {
        // Instance2D: compute positions from vertex_id (no stage_in); use quad index buffer
        // so vertex_id receives the values [0,1,2,1,3,2].
        [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                        indexCount:(NSUInteger)MeshResource->IndicesCPU.NumElements
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:mesh->indexBuffer
                 indexBufferOffset:0
                     instanceCount:instCount];
    }
}

void RenderResourceInstancedMac::Render(RenderInterface* rnd)
{
    // Upload underlying mesh + texture
    PrepareRender(rnd);
    RenderMac* r = static_cast<RenderMac*>(rnd);
    DrawInstanced(r, r->currentEncoder_);
}

#endif // VTS_PLATFORM_OSX

