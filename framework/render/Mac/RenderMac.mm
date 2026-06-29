#include <Platform.h>

#if VTS_PLATFORM_OSX
#include "render/Mac/RenderMac.h"
#include "render/Mac/WindowMac.h"
#include <assert.h>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* context);

template<typename T>
static void ReleaseObjcObject(T& obj)
{
#if !__has_feature(objc_arc)
    if (obj)
        [obj release];
#endif
    obj = nil;
}

// Metal shader moved to external file: framework/assets/mac_shader/shaders.metal
// The shader source is loaded at runtime by CreateShaderLibrary().

// (MSL source is stored in framework/assets/mac_shader/shaders.metal)

// ===========================================================================
// Helper: PSO cache key
// ===========================================================================
static int MakePSOKey(bool blend, bool depth, RenderShaderResource::ShaderTypes shader)
{
    return (int)shader | ((int)blend << 4) | ((int)depth << 5);
}

// ===========================================================================
// RenderInterface factory
// ===========================================================================
RenderInterface* RenderInterface::CreateRender()
{
    return new RenderMac();
}

// ===========================================================================
// RenderMac – construction / destruction
// ===========================================================================
RenderMac::RenderMac()
{
    frameSemaphore_ = dispatch_semaphore_create(QUEUE_SLOT_COUNT);
    memset(viewProj_, 0, sizeof(viewProj_));
    viewProj_[0][0] = viewProj_[1][1] = viewProj_[2][2] = viewProj_[3][3] = 1.0f;
}

RenderMac::~RenderMac()
{
    if (frameAutoreleasePool_)
    {
        objc_autoreleasePoolPop(frameAutoreleasePool_);
        frameAutoreleasePool_ = nullptr;
    }

    for (int type = 0; type < NumQuadDrawTypes; ++type)
    {
        for (int slot = 0; slot < QUEUE_SLOT_COUNT; ++slot)
            quadDraws_[type][slot].clear();
    }

    if (debugLineRes_)
    {
        delete debugLineRes_->MeshResource;
        delete debugLineRes_->ShaderResource;
        delete debugLineRes_;
        debugLineRes_ = nullptr;
    }
    if (debugSphereRes_)
    {
        delete debugSphereRes_->MeshResource;
        delete debugSphereRes_->ShaderResource;
        delete debugSphereRes_;
        debugSphereRes_ = nullptr;
    }

    delete quadMesh_;
    quadMesh_ = nullptr;
    for (auto& shader : quadShader_)
    {
        delete shader;
        shader = nullptr;
    }

    delete defaultTexture_;
    defaultTexture_ = nullptr;
    if (defaultFontTexture_)
    {
        delete defaultFontTexture_;
        defaultFontTexture_ = nullptr;
    }
    if (defaultFont_)
    {
        delete defaultFont_;
        defaultFont_ = nullptr;
    }

    for (auto& pso : psoPool_)
        ReleaseObjcObject(pso.second);
    psoPool_.clear();

    for (auto& depthState : depthStatePool_)
        ReleaseObjcObject(depthState.second);
    depthStatePool_.clear();

    ReleaseObjcObject(shaderLibrary_);
    ReleaseObjcObject(linearSampler_);
    ReleaseObjcObject(depthTexture_);
    ReleaseObjcObject(currentEncoder_);
    ReleaseObjcObject(currentCommandBuffer_);
    ReleaseObjcObject(currentDrawable_);
    ReleaseObjcObject(commandQueue_);
    ReleaseObjcObject(device_);

    delete window_;
    window_ = nullptr;
}

// ===========================================================================
// Shader library compilation
// ===========================================================================
void RenderMac::CreateShaderLibrary()
{
    // Derive the absolute path to shaders.metal from this source file's location.
    // __FILE__ = .../framework/render/Mac/RenderMac.mm  (compile-time absolute path)
    // shaders  = .../framework/assets/mac_shader/shaders.metal
    NSString* thisFile   = [NSString stringWithUTF8String:__FILE__];
    NSString* macDir     = [thisFile stringByDeletingLastPathComponent];          // .../render/Mac
    NSString* renderDir  = [macDir stringByDeletingLastPathComponent];            // .../render
    NSString* frameworkDir = [renderDir stringByDeletingLastPathComponent];       // .../framework
    NSString* shaderPath = [frameworkDir stringByAppendingPathComponent:@"assets/mac_shader/shaders.metal"];

    NSError* err = nil;
    NSString* src = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:&err];
    if (!src)
    {
        NSLog(@"Failed to load Metal shader file at %@: %@", shaderPath, err.localizedDescription);
        assert(false);
    }

    shaderLibrary_ = [device_ newLibraryWithSource:src options:nil error:&err];
    if (!shaderLibrary_)
    {
        NSLog(@"Metal shader compile error: %@", err.localizedDescription);
        assert(false);
    }
}

// ===========================================================================
// PSO + depth state creation
// ===========================================================================
id<MTLRenderPipelineState> RenderMac::GetOrCreatePSO(
    bool blendEnable, bool depthEnable,
    RenderShaderResource::ShaderTypes shaderType)
{
    int key = MakePSOKey(blendEnable, depthEnable, shaderType);
    auto it = psoPool_.find(key);
    if (it != psoPool_.end())
        return it->second;

    // Choose functions
    NSString* vsName = nil;
    NSString* fsName = nil;
    bool useVertexDescriptor = true;

    switch (shaderType)
    {
    case RenderShaderResource::ShaderType_OpaqueBasic:
        vsName = @"vs_opaque_basic";
        fsName = @"fs_opaque_basic";
        break;
    case RenderShaderResource::ShaderType_Instance3D:
        vsName = @"vs_instance_3d";
        fsName = @"fs_instanced";
        break;
    case RenderShaderResource::ShaderType_Instance2D:
        vsName              = @"vs_instance_2d";
        fsName              = @"fs_instanced";
        useVertexDescriptor = false;  // uses vertex_id only
        break;
    case RenderShaderResource::ShaderType_Skybox:
        vsName = @"vs_skybox";
        fsName = @"fs_skybox";
        break;
    default:
        assert(false);
        break;
    }

    id<MTLFunction> vs = [shaderLibrary_ newFunctionWithName:vsName];
    id<MTLFunction> fs = [shaderLibrary_ newFunctionWithName:fsName];
    assert(vs && fs);

    MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction   = vs;
    desc.fragmentFunction = fs;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    desc.depthAttachmentPixelFormat   = MTLPixelFormatDepth32Float_Stencil8;
    desc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    desc.rasterSampleCount = 1;

    // Alpha blending
    if (blendEnable)
    {
        desc.colorAttachments[0].blendingEnabled             = YES;
        desc.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
        desc.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
        desc.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
        desc.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorOne;
        desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
        desc.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;
    }

    // Vertex descriptor (for stage_in shaders only)
    if (useVertexDescriptor)
    {
        MTLVertexDescriptor* vtx = [MTLVertexDescriptor new];
        // attribute(0) = position  float3 @ offset 0
        vtx.attributes[0].format      = MTLVertexFormatFloat3;
        vtx.attributes[0].offset      = 0;
        vtx.attributes[0].bufferIndex = 0;
        // attribute(1) = vtxcolor  float3 @ offset 12
        vtx.attributes[1].format      = MTLVertexFormatFloat3;
        vtx.attributes[1].offset      = 12;
        vtx.attributes[1].bufferIndex = 0;
        // attribute(2) = uv        float2 @ offset 24
        vtx.attributes[2].format      = MTLVertexFormatFloat2;
        vtx.attributes[2].offset      = 24;
        vtx.attributes[2].bufferIndex = 0;
        // attribute(3) = normal float3 @ offset 32
        vtx.attributes[3].format = MTLVertexFormatFloat3;
        vtx.attributes[3].offset = 32;
        vtx.attributes[3].bufferIndex = 0;
        // layout 0: stride = sizeof(MeshVertex) = 44 bytes
        // (position[3] + vtxcolor[3] + uv[2] + normal[3])
        vtx.layouts[0].stride       = sizeof(RenderMeshResource::MeshVertex);
        vtx.layouts[0].stepRate     = 1;
        vtx.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        desc.vertexDescriptor = vtx;
        ReleaseObjcObject(vtx);
    }

    NSError* err = nil;
    id<MTLRenderPipelineState> pso = [device_ newRenderPipelineStateWithDescriptor:desc
                                                                              error:&err];
    ReleaseObjcObject(desc);
    ReleaseObjcObject(vs);
    ReleaseObjcObject(fs);
    if (!pso)
    {
        NSLog(@"Metal PSO creation failed: %@", err.localizedDescription);
        assert(false);
    }

    psoPool_[key] = pso;
    return pso;
}

id<MTLDepthStencilState> RenderMac::GetOrCreateDepthState(bool depthTest, bool depthWrite, MTLCompareFunction depthFunc)
{
    int key = (int)depthTest | ((int)depthWrite << 1) | ((int)depthFunc << 2);
    auto it = depthStatePool_.find(key);
    if (it != depthStatePool_.end())
        return it->second;

    MTLDepthStencilDescriptor* desc = [MTLDepthStencilDescriptor new];
    desc.depthCompareFunction = depthTest ? depthFunc : MTLCompareFunctionAlways;
    desc.depthWriteEnabled    = depthWrite;

    id<MTLDepthStencilState> ds = [device_ newDepthStencilStateWithDescriptor:desc];
    ReleaseObjcObject(desc);
    depthStatePool_[key] = ds;
    return ds;
}

// ===========================================================================
// Init – window, device, and built-in resources
// ===========================================================================
int RenderMac::Init()
{
    @autoreleasepool {
        // Create Cocoa window
        window_ = new WindowMac("Framework (Metal)", 1280, 720);

        // Metal device
        device_       = MTLCreateSystemDefaultDevice();
        commandQueue_ = [device_ newCommandQueue];
        assert(device_ && commandQueue_);

        // Configure CAMetalLayer on the window
        CAMetalLayer* layer        = window_->metalLayer;
        layer.device               = device_;
        layer.pixelFormat          = MTLPixelFormatBGRA8Unorm_sRGB;
        layer.framebufferOnly      = YES;
        // layer.drawableSize         = CGSizeMake(1280, 1280); // temporary; set below
        layer.drawableSize         = CGSizeMake(window_->width, window_->height);

        // Depth texture (fixed to window size for this training framework)
        {
            MTLTextureDescriptor* depthDesc =
                [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                                   width:(NSUInteger)window_->width
                                                                  height:(NSUInteger)window_->height
                                                               mipmapped:NO];
            depthDesc.usage       = MTLTextureUsageRenderTarget;
            depthDesc.storageMode = MTLStorageModePrivate;
            depthTexture_ = [device_ newTextureWithDescriptor:depthDesc];
        }

        // Linear sampler (linear min/mag, no mip)
        {
            MTLSamplerDescriptor* sd = [MTLSamplerDescriptor new];
            sd.minFilter    = MTLSamplerMinMagFilterLinear;
            sd.magFilter    = MTLSamplerMinMagFilterLinear;
            sd.mipFilter    = MTLSamplerMipFilterNotMipmapped;
            sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
            sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
            linearSampler_  = [device_ newSamplerStateWithDescriptor:sd];
            ReleaseObjcObject(sd);
        }

        // Compile shaders
        CreateShaderLibrary();

        // Default white texture
        defaultTexture_ = CreateRenderTextureResource();
        defaultTexture_->CreateDefaultTextureData();
        static_cast<RenderTextureResourceMac*>(defaultTexture_)->UploadToGPU(this);

        // Built-in resources (debug draw, quad batching)
        CreateBuiltInResources();
        m_imguiManager.init(device_,window_);
    }
    
    return 0;
}

// ===========================================================================
// Built-in resources (debug lines, debug spheres, quad batches)
// ===========================================================================
void RenderMac::CreateBuiltInResources()
{
    // Debug lines
    debugLineRes_ = new RenderResourceMac();
    debugLineRes_->PrimitiveType    = RenderResource::Primitive_LineList;
    debugLineRes_->TextureResource  = defaultTexture_;
    debugLineRes_->MeshResource     = CreateRenderMeshResource();
    debugLineRes_->ShaderResource   = CreateRenderShaderResource(RenderShaderResource::ShaderType_OpaqueBasic);

    auto& verts = debugLineRes_->MeshResource->VerticesCPU;
    auto& idxs  = debugLineRes_->MeshResource->IndicesCPU;
    verts.NumElements = MAX_DEBUG_DRAW_LINES * 2;
    verts.AllocBytes  = sizeof(RenderMeshResource::MeshVertex) * verts.NumElements;
    verts.Data        = (RenderMeshResource::MeshVertex*)malloc(verts.AllocBytes);
    idxs.NumElements  = MAX_DEBUG_DRAW_LINES * 2;
    idxs.AllocBytes   = sizeof(int) * idxs.NumElements;
    idxs.Data         = (int*)malloc(idxs.AllocBytes);
    debugLineRes_->MeshResource->IsDynamicMesh = true;
    for (int k = 0; k < MAX_DEBUG_DRAW_LINES * 2; ++k)
        idxs.Data[k] = k;

    // Debug spheres (instanced 3D)
    debugSphereRes_ = new RenderResourceInstancedMac();
    debugSphereRes_->TextureResource = defaultTexture_;
    debugSphereRes_->MeshResource    = CreateRenderMeshResource();
    debugSphereRes_->MeshResource->LoadMesh_Sphere(0.5f, 16, 16);
    debugSphereRes_->ShaderResource  = CreateRenderShaderResource(RenderShaderResource::ShaderType_Instance3D);
    debugSphereRes_->PrimitiveType   = RenderResource::Primitive_LineList;
    debugSphereRes_->m_InstancedData3D.ReallocateCPUBuffer(1024);
    debugSphereRes_->m_InstancedData2D.ReallocateCPUBuffer(1024);

    // Quad mesh (shared by instanced 2D/3D quad draws)
    quadMesh_ = static_cast<RenderMeshResourceMac*>(CreateRenderMeshResource());
    quadMesh_->LoadMesh_Plane();

    // Quad shaders per type
    quadShader_[QuadDraw2D]              = new RenderShaderResourceMac(RenderShaderResource::ShaderType_Instance2D);
    quadShader_[QuadDraw3D_DepthEnable]  = new RenderShaderResourceMac(RenderShaderResource::ShaderType_Instance3D);
    quadShader_[QuadDraw3D_DepthDisable] = new RenderShaderResourceMac(RenderShaderResource::ShaderType_Instance3D);
}

// ===========================================================================
// BeginFrame
// ===========================================================================
void RenderMac::BeginFrame()
{
    if (!frameAutoreleasePool_)
        frameAutoreleasePool_ = objc_autoreleasePoolPush();

    // Pump window events each frame
    if (window_) window_->PumpEvents();

    // Throttle CPU to at most QUEUE_SLOT_COUNT frames ahead of GPU
    dispatch_semaphore_wait(frameSemaphore_, DISPATCH_TIME_FOREVER);

    // Retire old quad batches for this slot
    for (int q = 0; q < NumQuadDrawTypes; ++q)
    {
        auto& batch = quadDraws_[q][currentSlot_];
        batch.clear();
    }

    // Acquire drawable
    currentDrawable_ = [window_->metalLayer nextDrawable];
    assert(currentDrawable_);

    // Command buffer
    currentCommandBuffer_ = [commandQueue_ commandBuffer];

    // Render pass descriptor
    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor new];
    rpd.colorAttachments[0].texture     = currentDrawable_.texture;
    rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
    rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.042, 0.042, 0.042, 1.0);

    rpd.depthAttachment.texture     = depthTexture_;
    rpd.depthAttachment.loadAction  = MTLLoadActionClear;
    rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
    rpd.depthAttachment.clearDepth  = 1.0;

    rpd.stencilAttachment.texture     = depthTexture_;
    rpd.stencilAttachment.loadAction  = MTLLoadActionClear;
    rpd.stencilAttachment.storeAction = MTLStoreActionDontCare;
    rpd.stencilAttachment.clearStencil = 0;

    currentEncoder_ = [currentCommandBuffer_ renderCommandEncoderWithDescriptor:rpd];
    m_imguiManager.beginFrame(rpd);
    ReleaseObjcObject(rpd);

    // Viewport
    MTLViewport vp = {0, 0, (double)window_->width, (double)window_->height, 0.0, 1.0};
    [currentEncoder_ setViewport:vp];

    ++frameNumber_;
}

// ===========================================================================
// EndFrame
// ===========================================================================
void RenderMac::EndFrame()
{
    // Flush deferred draws before closing the encoder
    FlushEndFrameDraws();
    
    m_imguiManager.endFrame(currentCommandBuffer_, currentEncoder_);

    [currentEncoder_ endEncoding];
    [currentCommandBuffer_ presentDrawable:currentDrawable_];

    // Signal the semaphore when GPU is done with this frame
    __block dispatch_semaphore_t sem = frameSemaphore_;
    [currentCommandBuffer_ addCompletedHandler:^(id<MTLCommandBuffer> /*buf*/) {
        dispatch_semaphore_signal(sem);
    }];

    [currentCommandBuffer_ commit];

    currentEncoder_       = nil;
    currentCommandBuffer_ = nil;
    currentDrawable_      = nil;

    if (frameAutoreleasePool_)
    {
        objc_autoreleasePoolPop(frameAutoreleasePool_);
        frameAutoreleasePool_ = nullptr;
    }

    currentSlot_ = (currentSlot_ + 1) % QUEUE_SLOT_COUNT;
}

// ===========================================================================
// Terminate
// ===========================================================================
void RenderMac::Terminate()
{
    m_imguiManager.shutdown();
    // Drain pending GPU work
    id<MTLCommandBuffer> sync = [commandQueue_ commandBuffer];
    [sync commit];
    [sync waitUntilCompleted];

    if (frameAutoreleasePool_)
    {
        objc_autoreleasePoolPop(frameAutoreleasePool_);
        frameAutoreleasePool_ = nullptr;
    }
}

// ===========================================================================
// Resource factories
// ===========================================================================
RenderResource* RenderMac::CreateRenderResource()
{
    return new RenderResourceMac();
}
RenderMeshResource* RenderMac::CreateRenderMeshResource()
{
    return new RenderMeshResourceMac();
}
RenderTextureResource* RenderMac::CreateRenderTextureResource()
{
    return new RenderTextureResourceMac();
}
RenderShaderResource* RenderMac::CreateRenderShaderResource(RenderShaderResource::ShaderTypes t)
{
    return new RenderShaderResourceMac(t);
}

RenderMeshResourceMac::~RenderMeshResourceMac()
{
    ReleaseObjcObject(vertexBuffer);
    ReleaseObjcObject(indexBuffer);
}

RenderTextureResourceMac::~RenderTextureResourceMac()
{
    ReleaseObjcObject(texture);
}

RenderResourceInstancedMac::~RenderResourceInstancedMac()
{
    for (int slot = 0; slot < 3; ++slot)
    {
        ReleaseObjcObject(instGpuBuf3D_[slot]);
        ReleaseObjcObject(instGpuBuf2D_[slot]);
    }
}

// ===========================================================================
// SetViewProjection / SetDefaultFont
// ===========================================================================
bool RenderMac::ShouldQuit()
{
    return window_ && window_->IsClosed();
}

void RenderMac::SetViewProjection(float(&viewProj)[4][4])
{
    memcpy(viewProj_, viewProj, sizeof(viewProj_));
}

void RenderMac::SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture)
{
    defaultFont_        = fontDesc;
    defaultFontTexture_ = texture;
    defaultFont_->PostLoadFont(texture->Width, texture->Height);
}

// ===========================================================================
// DebugDraw
// ===========================================================================
void RenderMac::DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB)
{
    DebugDraw::DrawArgs_Sphere s;
    memcpy(s.Pos,   posXYZ,   sizeof(s.Pos));
    memcpy(s.Scale, scaleXYZ, sizeof(s.Scale));
    memcpy(s.Color, colorRGB, sizeof(s.Color));
    debugSpheres_.push_back(s);
}

void RenderMac::DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB)
{
    DebugDraw::DrawArgs_Line li;
    memcpy(li.StartPos, pos0XYZ, sizeof(li.StartPos));
    memcpy(li.EndPos,   pos1XYZ, sizeof(li.EndPos));
    memcpy(li.Color,    colorRGB, sizeof(li.Color));
    debugLines_.push_back(li);
}

// ===========================================================================
// SetupDebugDrawSphere / Line
// ===========================================================================
size_t RenderMac::SetupDebugDrawSphere()
{
    auto& id3d = debugSphereRes_->m_InstancedData3D;
    id3d.ReallocateCPUBuffer((int)debugSpheres_.size());
    id3d.NumElementsCPU = (int)debugSpheres_.size();

    float uvOS[4] = {0, 0, 1, 1};
    for (int k = 0; k < (int)debugSpheres_.size(); ++k)
    {
        float* pos   = debugSpheres_[k].Pos;
        float* scale = debugSpheres_[k].Scale;
        float xform[4][4] = {
            {scale[0], 0, 0, 0},
            {0, scale[1], 0, 0},
            {0, 0, scale[2], 0},
            {pos[0], pos[1], pos[2], 1}
        };
        RenderBufferUtil::CopyShaderMatrix(id3d.DataCPU[k].xform, xform);
        memcpy(id3d.DataCPU[k].uv_offset_scale, uvOS, sizeof(uvOS));
        memcpy(id3d.DataCPU[k].color, debugSpheres_[k].Color, sizeof(float) * 3);
        id3d.DataCPU[k].color[3] = 1.0f;
    }
    size_t n = debugSpheres_.size();
    debugSpheres_.clear();
    return n;
}

size_t RenderMac::SetupDebugDrawLine()
{
    auto& mesh = *debugLineRes_->MeshResource;
    mesh.VerticesCPU.Resize((int)debugLines_.size() * 2);
    mesh.IndicesCPU.Resize((int)debugLines_.size() * 2);

    float uv[2] = {0, 0};
    for (int k = 0; k < (int)debugLines_.size() * 2; k += 2)
    {
        float* p0    = debugLines_[k / 2].StartPos;
        float* p1    = debugLines_[k / 2].EndPos;
        float* color = debugLines_[k / 2].Color;
        auto& v0 = mesh.VerticesCPU.Data[k];
        auto& v1 = mesh.VerticesCPU.Data[k + 1];
        memcpy(v0.position, p0, sizeof(float) * 3);
        memcpy(v0.uv,       uv, sizeof(float) * 2);
        memcpy(v0.vtxcolor, color, sizeof(float) * 3);
        memcpy(v1.position, p1, sizeof(float) * 3);
        memcpy(v1.uv,       uv, sizeof(float) * 2);
        memcpy(v1.vtxcolor, color, sizeof(float) * 3);
        mesh.IndicesCPU.Data[k]     = k;
        mesh.IndicesCPU.Data[k + 1] = k + 1;
    }
    size_t n = debugLines_.size();
    debugLines_.clear();
    return n;
}

// ===========================================================================
// FlushEndFrameDraws / FlushQuadDraws
// ===========================================================================
void RenderMac::FlushQuadDraws()
{
    for (int t = 0; t < NumQuadDrawTypes; ++t)
    {
        for (auto& res : quadDraws_[t][currentSlot_])
        {
            // Upload mesh/texture on first use
            static_cast<RenderMeshResourceMac*>(res->MeshResource)->UploadToGPU(this);
            static_cast<RenderTextureResourceMac*>(res->TextureResource)->UploadToGPU(this);

            RenderBufferUtil::CopyShaderMatrix(res->ConstBufferCPU.WorldViewProj, viewProj_);
            res->DrawInstanced(this, currentEncoder_);
        }
    }
}

void RenderMac::FlushEndFrameDraws()
{
    // Quads first
    FlushQuadDraws();

    // Debug spheres
    size_t numSpheres = SetupDebugDrawSphere();
    if (numSpheres > 0)
    {
        debugSphereRes_->MeshResource->IsDynamicMesh = false;
        static_cast<RenderMeshResourceMac*>(debugSphereRes_->MeshResource)->UploadToGPU(this);
        static_cast<RenderTextureResourceMac*>(debugSphereRes_->TextureResource)->UploadToGPU(this);
        RenderBufferUtil::CopyShaderMatrix(debugSphereRes_->ConstBufferCPU.WorldViewProj, viewProj_);
        debugSphereRes_->DrawInstanced(this, currentEncoder_);
    }

    // Debug lines
    size_t numLines = SetupDebugDrawLine();
    if (numLines > 0)
    {
        debugLineRes_->MeshResource->IsDynamicMesh = true;  // update each frame
        static_cast<RenderMeshResourceMac*>(debugLineRes_->MeshResource)->gpuDirty = true;
        debugLineRes_->PrepareRender(this);
        RenderBufferUtil::CopyShaderMatrix(debugLineRes_->ConstBufferCPU.WorldViewProj, viewProj_);
        debugLineRes_->DrawIndexed(this, currentEncoder_);
    }
}

// ===========================================================================
// Quad batching helpers
// ===========================================================================
RenderResourceInstancedMac* RenderMac::GetOrCreateQuadBatch(QuadDrawType type,
                                                              RenderTextureResource* texture)
{
    auto& batch = quadDraws_[type][currentSlot_];
    for (auto& res : batch)
        if (res->TextureResource == texture)
            return res.get();

    auto res = std::make_unique<RenderResourceInstancedMac>();
    res->MeshResource   = quadMesh_;
    res->ShaderResource = quadShader_[type];
    res->TextureResource = texture;
    res->EnableAlphaBlend = true;
    res->EnableDepthTest  = (type == QuadDraw3D_DepthEnable);
    batch.push_back(std::move(res));
    return batch.back().get();
}

// ===========================================================================
// DrawQuad3D / DrawQuad2D
// ===========================================================================
void RenderMac::DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4],
                            float(&color)[4], RenderTextureResource* texture, bool isDepthEnable)
{
    auto* res = GetOrCreateQuadBatch(isDepthEnable ? QuadDraw3D_DepthEnable : QuadDraw3D_DepthDisable,
                                     texture);
    res->m_InstancedData3D.TryReallocateCPUBuffer();
    auto& inst = res->m_InstancedData3D.DataCPU[res->m_InstancedData3D.NumElementsCPU++];
    RenderBufferUtil::CopyShaderMatrix(inst.xform, worldTransform);
    memcpy(inst.uv_offset_scale, uvOffsetScale, sizeof(uvOffsetScale));
    memcpy(inst.color, color, sizeof(color));
}

void RenderMac::DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4],
                            float(&color)[4], RenderTextureResource* texture)
{
    auto* res = GetOrCreateQuadBatch(QuadDraw2D, texture);
    res->m_InstancedData2D.TryReallocateCPUBuffer();
    auto& inst = res->m_InstancedData2D.DataCPU[res->m_InstancedData2D.NumElementsCPU++];
    RenderBufferUtil::CopyShaderMatrix(inst.xform, clipTransform);
    memcpy(inst.uv_offset_scale, uvOffsetScale, sizeof(uvOffsetScale));
    memcpy(inst.color, color, sizeof(color));
}

// ===========================================================================
// DrawText3D / DrawText2D / GetTextTotalWidth
// ===========================================================================
namespace {
struct DrawTextScratchPad
{
    float localXform[4][4];
    float worldXform[4][4];
    float uvOS[4];
};
static DrawTextScratchPad* GetScratchPad()
{
    static thread_local DrawTextScratchPad pad;
    return &pad;
}
}

void RenderMac::DrawText3D(const char* text, float(&worldTransform)[4][4],
                            float(&color)[4], float fontScale, bool isDepthEnable)
{
    int len = (int)strlen(text);
    assert(len < 1024);
    auto* pad = GetScratchPad();
    const float quadWidth = 1.0f;
    BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
    for (int k = 0; k < len; ++k)
    {
        defaultFont_->BuildInstanceData(ctx, text[k], pad->localXform, pad->uvOS);
        RenderBufferUtil::MulMatrix((float*)pad->localXform,
                                    (float*)worldTransform,
                                    (float*)pad->worldXform);
        DrawQuad3D(pad->worldXform, pad->uvOS, color, defaultFontTexture_, isDepthEnable);
    }
}

void RenderMac::DrawText2D(const char* text, float(&clipTransform)[4][4],
                            float(&color)[4], float fontScale)
{
    int len = (int)strlen(text);
    assert(len < 1024);
    auto* pad = GetScratchPad();
    const float quadWidth = 1.0f;
    BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
    for (int k = 0; k < len; ++k)
    {
        defaultFont_->BuildInstanceData(ctx, text[k], pad->localXform, pad->uvOS);
        RenderBufferUtil::MulMatrix((float*)pad->localXform,
                                    (float*)clipTransform,
                                    (float*)pad->worldXform);
        DrawQuad2D(pad->worldXform, pad->uvOS, color, defaultFontTexture_);
    }
}

float RenderMac::GetTextTotalWidth(const char* text, float fontScale)
{
    if (!defaultFont_) return 0.0f;
    const float quadWidth = 1.0f;
    return defaultFont_->GetTextTotalWidth(text, fontScale, quadWidth);
}

RenderTextureResource* RenderMac::CreateRenderDefaultTextureResource()
{
    assert(defaultTexture_ != nullptr);
    return defaultTexture_;
}

#endif // VTS_PLATFORM_OSX

