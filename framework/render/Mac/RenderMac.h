#pragma once
#include <Platform.h>

#if VTS_PLATFORM_OSX

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "render/RenderInterface.h"
#include "render/BMfont.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include "gameframework/log/ImGuiManager.h"

struct WindowMac;

// ---------------------------------------------------------------------------
// GPU-side structs (must match Metal shader declarations exactly)
// ---------------------------------------------------------------------------

// Matches the MSL ConstantBuffer struct (bound to buffer(1) in vertex shaders)
struct ConstantBufferMac
{
    float worldViewProj[4][4];  // uploaded after CopyShaderMatrix (transpose)
    float world[4][4];
    float cameraPos[4];
    float lightDir[4];
    float lightColor[4];
};

// Matches PerInstanceData3D in MSL (bound to buffer(2) for instanced 3D draws)
struct PerInstanceData3DMac
{
    float xform[4][4];          // CopyShaderMatrix-transposed
    float uv_offset_scale[4];
    float color[4];
};

// Matches PerInstanceData2D in MSL (bound to buffer(2) for instanced 2D draws)
struct PerInstanceData2DMac
{
    float xform[4][4];
    float uv_offset_scale[4];
    float color[4];
};

// ---------------------------------------------------------------------------
// Forward declarations of Mac resource subclasses
// ---------------------------------------------------------------------------
class RenderMac;
class RenderMeshResourceMac;
class RenderTextureResourceMac;
class RenderShaderResourceMac;
class RenderResourceMac;
class RenderResourceInstancedMac;

// ---------------------------------------------------------------------------
// RenderMeshResourceMac – owns vertex + index MTLBuffers
// ---------------------------------------------------------------------------
class RenderMeshResourceMac : public RenderMeshResource
{
    friend class RenderMac;
    friend class RenderResourceMac;
    friend class RenderResourceInstancedMac;
public:
    virtual ~RenderMeshResourceMac() override;
    void UploadToGPU(RenderMac* rnd);

    id<MTLBuffer> vertexBuffer   = nil;
    id<MTLBuffer> indexBuffer    = nil;
    uint32_t      vertexBytes    = 0;
    uint32_t      indexBytes     = 0;
    bool          gpuDirty       = true;  // upload needed
};

// ---------------------------------------------------------------------------
// RenderTextureResourceMac – owns an MTLTexture
// ---------------------------------------------------------------------------
class RenderTextureResourceMac : public RenderTextureResource
{
    friend class RenderMac;
    friend class RenderResourceMac;
public:
    virtual ~RenderTextureResourceMac() override;
    void UploadToGPU(RenderMac* rnd);

    id<MTLTexture> texture = nil;
};

// ---------------------------------------------------------------------------
// RenderShaderResourceMac – thin wrapper, just carries the ShaderType enum
// ---------------------------------------------------------------------------
class RenderShaderResourceMac : public RenderShaderResource
{
public:
    explicit RenderShaderResourceMac(ShaderTypes t) : RenderShaderResource(t) {}
    virtual ~RenderShaderResourceMac() override = default;
};

// ---------------------------------------------------------------------------
// RenderResourceMac – manages PrepareRender / Render / ExitRender
// ---------------------------------------------------------------------------
class RenderResourceMac : public RenderResource
{
    friend class RenderMac;
public:
    virtual void PrepareRender(RenderInterface* rnd) override;
    virtual void Render(RenderInterface* rnd) override;
    virtual void ExitRender(RenderInterface* rnd, bool waitForRenderDone) override;

protected:
    virtual void DrawIndexed(RenderMac* rnd, id<MTLRenderCommandEncoder> enc);
};

// ---------------------------------------------------------------------------
// RenderResourceInstancedMac – instanced 3D / 2D draws (quads, debug spheres)
// ---------------------------------------------------------------------------
class RenderResourceInstancedMac : public RenderResourceMac
{
    friend class RenderMac;
public:
    virtual ~RenderResourceInstancedMac() override;
    virtual void Render(RenderInterface* rnd) override;

    // CPU-side instance data (matches D3D12 implementation patterns)
    template<typename T>
    struct InstanceBuffer
    {
        T*   DataCPU        = nullptr;
        int  NumElementsCPU = 0;
        int  AllocCountCPU  = 0;

        void Reset()
        {
            free(DataCPU); DataCPU = nullptr;
            NumElementsCPU = AllocCountCPU = 0;
        }
        ~InstanceBuffer() { Reset(); }

        void ReallocateCPUBuffer(int n)
        {
            if (n > AllocCountCPU)
            {
                DataCPU = (T*)realloc(DataCPU, n * sizeof(T));
                AllocCountCPU = n;
            }
        }

        void TryReallocateCPUBuffer(int grow = 16)
        {
            if (NumElementsCPU + 1 > AllocCountCPU)
                ReallocateCPUBuffer(((NumElementsCPU + grow) / grow) * grow);
        }

        uint32_t DataBytes() const { return (uint32_t)(NumElementsCPU * sizeof(T)); }
    };

    InstanceBuffer<PerInstanceData3DMac> m_InstancedData3D;
    InstanceBuffer<PerInstanceData2DMac> m_InstancedData2D;

    // Persistent per-slot GPU buffers – written into each frame instead of
    // allocating a new MTLBuffer per draw.  Indexed by RenderMac::currentSlot_.
    // Grown (reallocated) only when the CPU-side data exceeds current capacity.
    id<MTLBuffer> instGpuBuf3D_[3] = {};
    uint32_t      instGpuBuf3DCapacity_[3] = {};
    id<MTLBuffer> instGpuBuf2D_[3] = {};
    uint32_t      instGpuBuf2DCapacity_[3] = {};

protected:
    void DrawInstanced(RenderMac* rnd, id<MTLRenderCommandEncoder> enc);
};

// ---------------------------------------------------------------------------
// RenderMac – main Metal renderer implementing RenderInterface
// ---------------------------------------------------------------------------
class RenderMac : public RenderInterface
{
    friend class RenderResourceMac;
    friend class RenderResourceInstancedMac;
    friend class RenderMeshResourceMac;
    friend class RenderTextureResourceMac;
public:
    RenderMac();
    virtual ~RenderMac();

    virtual int  Init()      override;
    virtual void BeginFrame() override;
    virtual void EndFrame()  override;
    virtual void Terminate() override;

    virtual RenderResource*        CreateRenderResource()           override;
    virtual RenderMeshResource*    CreateRenderMeshResource()       override;
    virtual RenderTextureResource* CreateRenderTextureResource()    override;
    virtual RenderShaderResource*  CreateRenderShaderResource(RenderShaderResource::ShaderTypes) override;

    virtual void  DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB) override;
    virtual void  DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB) override;
    virtual void  SetViewProjection(float(&viewProj)[4][4]) override;
    virtual void  SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture) override;
    virtual void  DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4],
                             RenderTextureResource* texture, bool isDepthEnable) override;
    virtual void  DrawText3D(const char* text, float(&worldTransform)[4][4], float(&color)[4],
                             float fontScale, bool isDepthEnable) override;
    virtual void  DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4],
                             RenderTextureResource* texture) override;
    virtual void  DrawText2D(const char* text, float(&clipTransform)[4][4], float(&color)[4], float fontScale) override;
    virtual float GetTextTotalWidth(const char* text, float fontScale) override;
    virtual bool  ShouldQuit() override;
    virtual RenderTextureResource* CreateRenderDefaultTextureResource() override;
    // Public Metal objects (accessed by resource classes)
    id<MTLDevice>              device_        = nil;
    id<MTLCommandQueue>        commandQueue_  = nil;
    id<MTLRenderCommandEncoder> currentEncoder_ = nil;
    id<MTLCommandBuffer>       currentCommandBuffer_ = nil;
    id<CAMetalDrawable>        currentDrawable_ = nil;

    static const int QUEUE_SLOT_COUNT    = 3;
    static const int MAX_DEBUG_DRAW_LINES = 1024;

    int      currentSlot_ = 0;
    uint64_t frameNumber_ = 1;

    id<MTLSamplerState> linearSampler_ = nil;
    id<MTLTexture>      depthTexture_  = nil;

    id<MTLRenderPipelineState> GetOrCreatePSO(bool blendEnable, bool depthEnable,
                                               RenderShaderResource::ShaderTypes shaderType);
    id<MTLDepthStencilState>   GetOrCreateDepthState(bool depthEnable, bool depthWrite, MTLCompareFunction depthFunc = MTLCompareFunctionLess);

private:
    // Shader source + library
    id<MTLLibrary> shaderLibrary_ = nil;
    void           CreateShaderLibrary();

    // PSO / depth-state caches
    std::unordered_map<int, id<MTLRenderPipelineState>> psoPool_;
    std::unordered_map<int, id<MTLDepthStencilState>>   depthStatePool_;

    // Per-frame triple-buffer semaphore
    dispatch_semaphore_t frameSemaphore_;
    void*                frameAutoreleasePool_ = nullptr;

    // Window
    WindowMac* window_ = nullptr;

    // Built-in resources
    RenderTextureResource*       defaultTexture_     = nullptr;
    BMFont*                      defaultFont_        = nullptr;
    RenderTextureResource*       defaultFontTexture_ = nullptr;
    float                        viewProj_[4][4]     = {};

    // Debug draw deferred lists
    std::vector<DebugDraw::DrawArgs_Sphere> debugSpheres_;
    std::vector<DebugDraw::DrawArgs_Line>   debugLines_;

    // Debug and quad resources
    RenderResourceMac*           debugLineRes_   = nullptr;
    RenderResourceInstancedMac*  debugSphereRes_ = nullptr;
    RenderMeshResourceMac*       quadMesh_       = nullptr;

    enum QuadDrawType
    {
        QuadDraw2D,
        QuadDraw3D_DepthEnable,
        QuadDraw3D_DepthDisable,
        NumQuadDrawTypes
    };

    // Pooled instanced batches per quad type per frame slot
    std::vector<std::unique_ptr<RenderResourceInstancedMac>>
        quadDraws_[NumQuadDrawTypes][QUEUE_SLOT_COUNT];

    RenderShaderResourceMac* quadShader_[NumQuadDrawTypes] = {};

    // Helpers
    void CreateBuiltInResources();
    RenderResourceInstancedMac* GetOrCreateQuadBatch(QuadDrawType type,
                                                     RenderTextureResource* texture);
    void FlushQuadDraws();
    void FlushEndFrameDraws();
    size_t SetupDebugDrawSphere();
    size_t SetupDebugDrawLine();
    
    ImGuiManager m_imguiManager;
};

#endif // VTS_PLATFORM_OSX
