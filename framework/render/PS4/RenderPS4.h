
#pragma once

#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <gnmx.h>
#include <vector>
#include <map>
#include <memory>
#include "render/PS4/common/allocator.h"
#include "render/RenderInterface.h"

class NativeDevicePS4;
class RenderResourcePS4;
class RenderResourceInstancedPS4;
class RenderMeshResourcePS4;
class RenderShaderResourcePS4;
class BMFont;

class FrameResourcePS4 : public FrameResource
{
	friend class RenderResourcePS4;
	friend class RenderResourceInstancedPS4;
	friend class RenderPS4;

public:
	bool Init(NativeDevicePS4* dvRes);

protected:
	sce::Gnmx::GnmxGfxContext    gfxContext;
#if SCE_GNMX_ENABLE_GFX_LCUE
	void* resourceBuffer;
	void* dcbBuffer;
#else
	void* cueHeap;
	void* dcbBuffer;
	void* ccbBuffer;
#endif
	volatile uint32_t* contextLabel;
};

typedef struct DisplayBuffer
{
	sce::Gnm::RenderTarget       renderTarget;
	int                     displayIndex;
}
DisplayBuffer;

class NativeDevicePS4 : public NativeDevice
{
	friend class RenderResourcePS4;
	friend class RenderMeshResourcePS4;
	friend class RenderTextureResourcePS4;
	friend class RenderShaderResourcePS4;
	friend class RenderResourceInstancedPS4;
	friend class RenderPS4;
	friend class FrameResourcePS4;
public:
	bool InitAllocator();
	bool InitRenderTargets();
	bool InitBackBuffer();
	void FlipBackbuffer();
	bool InitSamplers();
	bool InitDepthBlend();
	bool HasRenderDone(uint64_t frameNumber);

	NativeDevicePS4()
	{
		for (int t = 0; t < kDisplayBufferCount; ++t)
		{
			m_DisplayBuffers[t] = new DisplayBuffer();
		}
	}

	virtual ~NativeDevicePS4()
	{
		for (int t = 0; t < kDisplayBufferCount; ++t)
		{
			delete m_DisplayBuffers[t];
			m_DisplayBuffers[t] = nullptr;
		}
	}

protected:
	void FreeGarlicMemory(unsigned int size, unsigned int alignment, void* ptr);
	void* AllocGarlicMemory(unsigned int sizeReqested, unsigned int alignment, unsigned int& sizeAllocated);
	void FlushPendingFreeGarlicMemory();

	SceKernelEqueue m_EopEventQueue;
	sce::Gnm::DepthRenderTarget m_DepthTarget;
	int m_VideoOutHandle;
	LinearAllocator m_OnionAllocator;
	LinearAllocator m_GarlicAllocator;
	sce::Gnmx::Toolkit::Allocators m_ToolkitAllocators;
	sce::Gnm::GpuMode m_GpuMode;

	sce::Gnm::BlendControl m_BlendControl[2];
	sce::Gnm::DepthStencilControl m_DepthStencilControl[2];

	static const int kDisplayBufferCount = 3;
	DisplayBuffer* m_DisplayBuffers[kDisplayBufferCount];
	DisplayBuffer* m_BackBuffer = nullptr;
	int m_BackBufferIndex = 0;

	volatile uint64_t* m_FrameNumberGPU = nullptr;
	uint64_t m_FrameNumberCPU = 0;

	sce::Gnm::Sampler m_Sampler;

	struct VRAMBlock
	{
		unsigned int Size;
		unsigned int Alignment;
		void* GPUAddress;
	};
	std::vector<VRAMBlock> GarlicAllocatorFreeList;
	std::vector<VRAMBlock> GarlicAllocatorPendingFree;
};

class RenderPS4 : public RenderInterface
{
	friend class RenderResourcePS4;
	friend class RenderResourceInstancedPS4;
	friend class RenderMeshResourcePS4;
	friend class RenderTextureResourcePS4;
	friend class RenderShaderResourcePS4;
public:
	RenderPS4();
	virtual ~RenderPS4();
	virtual int Init() override;
	virtual void BeginFrame() override;
	virtual void EndFrame() override;
	virtual void Terminate() override;
	virtual RenderResource* CreateRenderResource() override;
	virtual RenderMeshResource* CreateRenderMeshResource() override;
	virtual RenderTextureResource* CreateRenderTextureResource() override;
	virtual RenderShaderResource* CreateRenderShaderResource(RenderShaderResource::ShaderTypes _type) override;

	static const int MAX_NUM_DEBUG_DRAW_LINES = 1024;
	virtual void DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB) override;
	virtual void DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB) override;
	virtual void SetViewProjection(float(&viewProj)[4][4]) override;
	virtual void SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture) override;
	virtual void DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE) override;
	virtual void DrawText3D(const char* text, float(&worldTransform)[4][4], float(&color)[4], float fontScale, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE) override;
	// use 4x4 matrix for 3x3
	// {
	//	scaleRotation,	scaleRotation,	0,				0,
	//	scaleRotation,	scaleRotation,	0,				0,
	//	0,				0,				1,				0,
	//	posX,			posY,			0,				1
	// }
	virtual void DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture) override;
	virtual void DrawText2D(const char* text, float(&clipTransform)[4][4], float(&color)[4], float fontScale) override;
	virtual float GetTextTotalWidth(const char* text, float fontScale) override;
protected:
	enum QuadDrawType
	{
		QuadDraw2D,
		QuadDraw3D_DepthEnable,
		QuadDraw3D_DepthDisable,

		NumQuadDrawTypes
	};
	void CreateBuiltInDrawResources();
	void ReleaseOwnedResources();
	int SetupDebugDrawSphere();
	int SetupDebugDrawLine();
	void FlushEndFrameDraws(sce::Gnmx::GnmxGfxContext& gfxc);
	void FlushQuadDraws(sce::Gnmx::GnmxGfxContext& gfxc);
	RenderResourceInstancedPS4* GetOrCreateNextRenderResourceInstanced(QuadDrawType drawType, RenderTextureResource* texture);

	static const int kFrameResourceCount = 2;
	FrameResourcePS4* m_FrameResources[kFrameResourceCount];
	FrameResourcePS4* m_FrameResource = nullptr;

	NativeDevicePS4* m_NativeDeviceResource = nullptr;
	int m_FrameResourcesIndex = 0;

	RenderTextureResource* DefaultTexture;
	BMFont* m_DefaultFont = nullptr;
	RenderTextureResource* m_DefaultFontTexture = nullptr;

	enum SRVTableDefines
	{
		SRVTable_Texture_t0,
		SRVTable_Texture_t1,
		SRVTable_Texture_t2,
		SRVTable_Texture_t3,
		SRVTable_InstanceBuffer3D_t4,
		SRVTable_InstanceBuffer2D_t5,

		NumSRVTableEntries,
	};

	float ViewProj[4][4] =
	{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};

	std::vector<DebugDraw::DrawArgs_Line>  DebugDrawLines;
	std::vector<DebugDraw::DrawArgs_Sphere>  DebugDrawSpheres;

	RenderResourcePS4* m_DebugDrawLineResource = nullptr;
	RenderResourceInstancedPS4* m_DebugDrawSphereResource = nullptr;
	RenderMeshResourcePS4* m_QuadMesh = nullptr;
	RenderShaderResourcePS4* m_QuadShader[NumQuadDrawTypes] = {nullptr, nullptr};
	std::vector<std::unique_ptr<RenderResourceInstancedPS4>> m_QuadDraws[NumQuadDrawTypes];
};

extern  uint32_t kDisplayBufferWidth;
extern uint32_t kDisplayBufferHeight;

#endif