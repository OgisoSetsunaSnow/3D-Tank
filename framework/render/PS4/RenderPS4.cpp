#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <stdio.h>
#include <stdlib.h>
#include <scebase.h>
#include <kernel.h>
#include <assert.h>
#include <video_out.h>
#include <vectormath.h>
#include "render/PS4/api_gnm/toolkit/toolkit.h"
#include "render/RenderInterface.h"
#include "render/PS4/RenderPS4.h"
#include "render/PS4/RenderResourcePS4.h"
#include "render/BMfont.h"

using namespace sce;
using namespace sce::Gnmx;

size_t sceLibcHeapSize = 64 * 1024 * 1024;

uint32_t kDisplayBufferWidth = 1920;
uint32_t kDisplayBufferHeight = 1080;
static const Gnm::ZFormat kZFormat = Gnm::kZFormat32Float;
static const Gnm::StencilFormat kStencilFormat = Gnm::kStencil8;
static const bool kHtileEnabled = true;
static const Vector4 kClearColor = Vector4(0.042f, 0.042f, 0.042f, 1);
static const size_t kOnionMemorySize = 64 * 4 * 1024 * 1024;
static const size_t kGarlicMemorySize = 64 * 4 * 1024 * 1024;

#if SCE_GNMX_ENABLE_GFX_LCUE
static const uint32_t kNumLcueBatches = 100;
static const size_t kDcbSizeInBytes = 16 * 1024 * 1024;
#else
static const uint32_t kCueRingEntries = 64;
static const size_t kDcbSizeInBytes = 16 * 1024 * 1024;
static const size_t kCcbSizeInBytes = 256 * 1024;
#endif

enum RenderContextState
{
	kRenderContextFree = 0,
	kRenderContextInUse,
};

RenderInterface* RenderInterface::CreateRender()
{
	return new RenderPS4();
}

RenderPS4::RenderPS4()
{
	for (int k = 0; k< kFrameResourceCount; ++k)
	{
		m_FrameResources[k] = new FrameResourcePS4();
	}

	m_NativeDeviceResource = new NativeDevicePS4();
}

void RenderPS4::ReleaseOwnedResources()
{
	for (int t = 0; t < NumQuadDrawTypes; ++t)
	{
		for (auto& res : m_QuadDraws[t])
		{
			if (res)
				res->ExitRender(this, false);
		}
		m_QuadDraws[t].clear();
	}

	if (m_DebugDrawLineResource)
	{
		m_DebugDrawLineResource->ExitRender(this, false);
		if (m_DebugDrawLineResource->MeshResource)
		{
			static_cast<RenderMeshResourcePS4*>(m_DebugDrawLineResource->MeshResource)->Release(this);
			delete m_DebugDrawLineResource->MeshResource;
			m_DebugDrawLineResource->MeshResource = nullptr;
		}
		if (m_DebugDrawLineResource->ShaderResource)
		{
			static_cast<RenderShaderResourcePS4*>(m_DebugDrawLineResource->ShaderResource)->ReleaseGPUResource(this);
			delete m_DebugDrawLineResource->ShaderResource;
			m_DebugDrawLineResource->ShaderResource = nullptr;
		}
		delete m_DebugDrawLineResource;
		m_DebugDrawLineResource = nullptr;
	}

	if (m_DebugDrawSphereResource)
	{
		m_DebugDrawSphereResource->ExitRender(this, false);
		if (m_DebugDrawSphereResource->MeshResource)
		{
			static_cast<RenderMeshResourcePS4*>(m_DebugDrawSphereResource->MeshResource)->Release(this);
			delete m_DebugDrawSphereResource->MeshResource;
			m_DebugDrawSphereResource->MeshResource = nullptr;
		}
		if (m_DebugDrawSphereResource->ShaderResource)
		{
			static_cast<RenderShaderResourcePS4*>(m_DebugDrawSphereResource->ShaderResource)->ReleaseGPUResource(this);
			delete m_DebugDrawSphereResource->ShaderResource;
			m_DebugDrawSphereResource->ShaderResource = nullptr;
		}
		delete m_DebugDrawSphereResource;
		m_DebugDrawSphereResource = nullptr;
	}

	if (m_QuadMesh)
	{
		m_QuadMesh->Release(this);
		delete m_QuadMesh;
		m_QuadMesh = nullptr;
	}

	for (int i = 0; i < NumQuadDrawTypes; ++i)
	{
		if (m_QuadShader[i])
		{
			m_QuadShader[i]->ReleaseGPUResource(this);
			delete m_QuadShader[i];
			m_QuadShader[i] = nullptr;
		}
	}

	if (m_DefaultFontTexture)
	{
		static_cast<RenderTextureResourcePS4*>(m_DefaultFontTexture)->Release(this);
		delete m_DefaultFontTexture;
		m_DefaultFontTexture = nullptr;
	}

	if (DefaultTexture)
	{
		static_cast<RenderTextureResourcePS4*>(DefaultTexture)->Release(this);
		delete DefaultTexture;
		DefaultTexture = nullptr;
	}
}

RenderPS4::~RenderPS4()
{
	ReleaseOwnedResources();
	if (m_DefaultFont)
	{
		delete m_DefaultFont;
		m_DefaultFont = nullptr;
	}
	for (int k = 0; k < kFrameResourceCount; ++k)
	{
		delete m_FrameResources[k];
		m_FrameResources[k] = nullptr;
	}

	delete m_NativeDeviceResource;
	m_NativeDeviceResource = nullptr;
}

int RenderPS4::Init()
{
	m_NativeDeviceResource->m_GpuMode = Gnm::getGpuMode();
	int ret;

	m_NativeDeviceResource->InitAllocator();

	// Open the video output port
	m_NativeDeviceResource->m_VideoOutHandle = sceVideoOutOpen(0, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);
	if (m_NativeDeviceResource->m_VideoOutHandle < 0)
	{
		printf("sceVideoOutOpen failed: 0x%08X\n", m_NativeDeviceResource->m_VideoOutHandle);
		return m_NativeDeviceResource->m_VideoOutHandle;
	}

	// Initialize the flip rate: 0: 60Hz, 1: 30Hz or 2: 20Hz
	ret = sceVideoOutSetFlipRate(m_NativeDeviceResource->m_VideoOutHandle, 0);
	if (ret != SCE_OK)
	{
		printf("sceVideoOutSetFlipRate failed: 0x%08X\n", ret);
		return ret;
	}

	// Create the event queue for used to synchronize with end-of-pipe interrupts
	ret = sceKernelCreateEqueue(&m_NativeDeviceResource->m_EopEventQueue, "EOP QUEUE");
	if (ret != SCE_OK)
	{
		printf("sceKernelCreateEqueue failed: 0x%08X\n", ret);
		return ret;
	}

	// Register for the end-of-pipe events
	ret = Gnm::addEqEvent(m_NativeDeviceResource->m_EopEventQueue, Gnm::kEqEventGfxEop, NULL);
	if (ret != SCE_OK)
	{
		printf("Gnm::addEqEvent failed: 0x%08X\n", ret);
		return ret;
	}

	// For simplicity reasons the sample uses a single GfxContext for each
	// frame. Implementing more complex schemes where multipleGfxContext-s
	// are submitted in each frame is possible as well, but it is out of the
	// scope for this basic sample.
	

	// Initialize all the render contexts
	for (uint32_t i = 0; i < kFrameResourceCount; ++i)
	{
		m_FrameResources[i]->Init(m_NativeDeviceResource);
	}

	m_NativeDeviceResource->InitBackBuffer();

	m_NativeDeviceResource->InitRenderTargets();

	m_NativeDeviceResource->InitSamplers();

	m_NativeDeviceResource->InitDepthBlend();

	m_FrameResource = m_FrameResources[0];

	DefaultTexture = (RenderTextureResourcePS4*)CreateRenderTextureResource();
	DefaultTexture->CreateDefaultTextureData();
	RenderTextureResourcePS4* defaultTexPS4 = (RenderTextureResourcePS4*)DefaultTexture;
	defaultTexPS4->SetupTexture(m_NativeDeviceResource);

	CreateBuiltInDrawResources();

	return 0;
}

void RenderPS4::BeginFrame()
{
	int ret;
	Gnmx::GnmxGfxContext& gfxc = m_FrameResource->gfxContext;

	// Wait until the context label has been written to make sure that the
	// GPU finished parsing the command buffers before overwriting them
	while (m_FrameResource->contextLabel[0] != kRenderContextFree)
	{
		// Wait for the EOP event
		SceKernelEvent eopEvent;
		int count;
		ret = sceKernelWaitEqueue(m_NativeDeviceResource->m_EopEventQueue, &eopEvent, 1, &count, NULL);
		if (ret != SCE_OK)
		{
			printf("sceKernelWaitEqueue failed: 0x%08X\n", ret);
		}
	}

	m_NativeDeviceResource->FlushPendingFreeGarlicMemory();

	++(m_NativeDeviceResource->m_FrameNumberCPU);

	// Reset the flip GPU label
	m_FrameResource->contextLabel[0] = kRenderContextInUse;

	// Reset the graphical context and initialize the hardware state
	gfxc.reset();
	gfxc.initializeDefaultHardwareState();

	// In a real-world scenario, any rendering of off-screen buffers or
	// other compute related processing would go here

	// The waitUntilSafeForRendering stalls the GPU until the scan-out
	// operations on the current display buffer have been completed.
	// This command is not blocking for the CPU.
	//
	// NOTE
	// This command should be used right before writing the display buffer.
	//
	gfxc.waitUntilSafeForRendering(m_NativeDeviceResource->m_VideoOutHandle, m_NativeDeviceResource->m_BackBuffer->displayIndex);

	// Setup the viewport to match the entire screen.
	// The z-scale and z-offset values are used to specify the transformation
	// from clip-space to screen-space
	gfxc.setupScreenViewport(
		0,			// Left
		0,			// Top
		m_NativeDeviceResource->m_BackBuffer->renderTarget.getWidth(),
		m_NativeDeviceResource->m_BackBuffer->renderTarget.getHeight(),
		0.5f,		// Z-scale
		0.5f);		// Z-offset

	// Bind the render & depth targets to the context
	gfxc.setRenderTarget(0, &m_NativeDeviceResource->m_BackBuffer->renderTarget);
	gfxc.setDepthRenderTarget(&m_NativeDeviceResource->m_DepthTarget);

	// Clear the color and the depth target
	Toolkit::SurfaceUtil::clearRenderTarget(gfxc, &m_NativeDeviceResource->m_BackBuffer->renderTarget, kClearColor);
	Toolkit::SurfaceUtil::clearDepthTarget(gfxc, &m_NativeDeviceResource->m_DepthTarget, m_NativeDeviceResource->DepthZDir == NativeDevice::FAR_IS_ZERO ? 0.0f : 1.f);
}

void RenderPS4::EndFrame()
{
	int ret;
	Gnmx::GnmxGfxContext& gfxc = m_FrameResource->gfxContext;
	FlushEndFrameDraws(gfxc);

	gfxc.writeAtEndOfPipe(Gnm::kEopCbDbReadsDone, Gnm::kEventWriteDestMemory, (void*)m_NativeDeviceResource->m_FrameNumberGPU, Gnm::kEventWriteSource64BitsImmediate, m_NativeDeviceResource->m_FrameNumberCPU, Gnm::kCacheActionNone, Gnm::kCachePolicyBypass);

	// Submit the command buffers, request a flip of the display buffer and
	// write the GPU label that determines the render context state (free)
	// and trigger a software interrupt to signal the EOP event queue
	// NOTE: for this basic sample we are submitting a single GfxContext
	// per frame. Submitting multiple GfxContext-s per frame is allowed.
	// Multiple contexts are processed in order, i.e.: they start in
	// submission order and end in submission order.
	ret = gfxc.submitAndFlipWithEopInterrupt(
		m_NativeDeviceResource->m_VideoOutHandle,
		m_NativeDeviceResource->m_BackBuffer->displayIndex,
		SCE_VIDEO_OUT_FLIP_MODE_VSYNC,
		0,
		sce::Gnm::kEopFlushCbDbCaches,
		const_cast<uint32_t*>(m_FrameResource->contextLabel),
		kRenderContextFree,
		sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2);
	if (ret != sce::Gnm::kSubmissionSuccess)
	{
		// Analyze the error code to determine whether the command buffers
		// have been submitted to the GPU or not
		if (ret & sce::Gnm::kStatusMaskError)
		{
			// Error codes in the kStatusMaskError family block submissions
			// so we need to mark this render context as not-in-flight
			m_FrameResource->contextLabel[0] = kRenderContextFree;
		}

		printf("GfxContext::submitAndFlip failed: 0x%08X\n", ret);
	}

	// Signal the system that every draw for this frame has been submitted.
	// This function gives permission to the OS to hibernate when all the
	// currently running GPU tasks (graphics and compute) are done.
	ret = Gnm::submitDone();
	if (ret != SCE_OK)
	{
		printf("Gnm::submitDone failed: 0x%08X\n", ret);
	}

	m_NativeDeviceResource->FlipBackbuffer();

	// Rotate the render contexts
	m_FrameResourcesIndex = (m_FrameResourcesIndex + 1) % kFrameResourceCount;
	m_FrameResource = m_FrameResources[m_FrameResourcesIndex];
}

bool NativeDevicePS4::InitAllocator()
{
	// Initialize the WB_ONION memory allocator
	int ret = m_OnionAllocator.initialize(
		kOnionMemorySize, SCE_KERNEL_WB_ONION,
		SCE_KERNEL_PROT_CPU_RW | SCE_KERNEL_PROT_GPU_ALL);
	if (ret != SCE_OK)
		return false;

	// Initialize the WC_GARLIC memory allocator
	// NOTE: CPU reads from GARLIC write-combined memory have a very low
	//       bandwidth but we need it for fence readback
	ret = m_GarlicAllocator.initialize(
		kGarlicMemorySize,
		SCE_KERNEL_WC_GARLIC,
		SCE_KERNEL_PROT_CPU_RW | SCE_KERNEL_PROT_GPU_ALL);
	if (ret != SCE_OK)
		return false;

	// Initialize the Toolkit module
	m_OnionAllocator.getIAllocator(m_ToolkitAllocators.m_onion);
	m_GarlicAllocator.getIAllocator(m_ToolkitAllocators.m_garlic);
	Toolkit::initializeWithAllocators(&m_ToolkitAllocators);

	return true;
}

bool NativeDevicePS4::InitRenderTargets()
{
	int ret;
	// Compute the tiling mode for the depth buffer
	Gnm::DataFormat depthFormat = Gnm::DataFormat::build(kZFormat);
	Gnm::TileMode depthTileMode;
	ret = GpuAddress::computeSurfaceTileMode(
		m_GpuMode, // NEO or Base
		&depthTileMode,									// Tile mode pointer
		GpuAddress::kSurfaceTypeDepthOnlyTarget,		// Surface type
		depthFormat,									// Surface format
		1);												// Elements per pixel
	if (ret != SCE_OK)
	{
		printf("GpuAddress::computeSurfaceTileMode: 0x%08X\n", ret);
		return false;
	}

	// Initialize the depth buffer descriptor
	Gnm::SizeAlign stencilSizeAlign;
	Gnm::SizeAlign htileSizeAlign;

	Gnm::DepthRenderTargetSpec spec;
	spec.init();
	spec.m_width = kDisplayBufferWidth;
	spec.m_height = kDisplayBufferHeight;
	spec.m_pitch = 0;
	spec.m_numSlices = 1;
	spec.m_zFormat = depthFormat.getZFormat();
	spec.m_stencilFormat = kStencilFormat;
	spec.m_minGpuMode = m_GpuMode;
	spec.m_numFragments = Gnm::kNumFragments1;
	spec.m_flags.enableHtileAcceleration = kHtileEnabled ? 1 : 0;

	ret = m_DepthTarget.init(&spec);
	if (ret != SCE_GNM_OK)
		return false;

	Gnm::SizeAlign depthTargetSizeAlign = m_DepthTarget.getZSizeAlign();

	// Initialize the HTILE buffer, if enabled
	if (kHtileEnabled)
	{
		htileSizeAlign = m_DepthTarget.getHtileSizeAlign();
		void* htileMemory = m_GarlicAllocator.allocate(htileSizeAlign);
		if (!htileMemory)
		{
			printf("Cannot allocate the HTILE buffer\n");
			//return SCE_KERNEL_ERROR_ENOMEM;
			return false;
		}

		m_DepthTarget.setHtileAddress(htileMemory);
	}

	// Initialize the stencil buffer, if enabled
	void* stencilMemory = NULL;
	stencilSizeAlign = m_DepthTarget.getStencilSizeAlign();
	if (kStencilFormat != Gnm::kStencilInvalid)
	{
		stencilMemory = m_GarlicAllocator.allocate(stencilSizeAlign);
		if (!stencilMemory)
		{
			printf("Cannot allocate the stencil buffer\n");
			//return SCE_KERNEL_ERROR_ENOMEM;
			return false;
		}
	}

	// Allocate the depth buffer
	void* depthMemory = m_GarlicAllocator.allocate(depthTargetSizeAlign);
	if (!depthMemory)
	{
		printf("Cannot allocate the depth buffer\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}
	m_DepthTarget.setAddresses(depthMemory, stencilMemory);

	return true;
}

bool NativeDevicePS4::InitBackBuffer()
{
	Gnm::SizeAlign fenceSizeAlign(sizeof(uint64_t), sizeof(uint64_t));
	m_FrameNumberGPU = (volatile uint64_t*)m_GarlicAllocator.allocate(fenceSizeAlign);
	*m_FrameNumberGPU = 0;

	m_BackBuffer = m_DisplayBuffers[0];

	// Convenience array used by sceVideoOutRegisterBuffers()
	void* surfaceAddresses[kDisplayBufferCount];

	SceVideoOutResolutionStatus status;
	if (SCE_OK == sceVideoOutGetResolutionStatus(m_VideoOutHandle, &status) && status.fullHeight > 1080)
	{
		kDisplayBufferWidth *= 2;
		kDisplayBufferHeight *= 2;
	}

	int ret;
	// Initialize all the display buffers
	for (uint32_t i = 0; i < kDisplayBufferCount; ++i)
	{
		// Compute the tiling mode for the render target
		Gnm::TileMode tileMode;
		Gnm::DataFormat format = Gnm::kDataFormatB8G8R8A8UnormSrgb;
		ret = GpuAddress::computeSurfaceTileMode(
			m_GpuMode, // NEO or base
			&tileMode,										// Tile mode pointer
			GpuAddress::kSurfaceTypeColorTargetDisplayable,	// Surface type
			format,											// Surface format
			1);												// Elements per pixel
		if (ret != SCE_OK)
		{
			printf("GpuAddress::computeSurfaceTileMode: 0x%08X\n", ret);
			return false;
		}

		// Initialize the render target descriptor

		Gnm::RenderTargetSpec spec;
		spec.init();
		spec.m_width = kDisplayBufferWidth;
		spec.m_height = kDisplayBufferHeight;
		spec.m_pitch = 0;
		spec.m_numSlices = 1;
		spec.m_colorFormat = format;
		spec.m_colorTileModeHint = tileMode;
		spec.m_minGpuMode = m_GpuMode;
		spec.m_numSamples = Gnm::kNumSamples1;
		spec.m_numFragments = Gnm::kNumFragments1;
		spec.m_flags.enableCmaskFastClear = 0;
		spec.m_flags.enableFmaskCompression = 0;
		ret = m_DisplayBuffers[i]->renderTarget.init(&spec);
		if (ret != SCE_GNM_OK)
			return false;

		Gnm::SizeAlign sizeAlign = m_DisplayBuffers[i]->renderTarget.getColorSizeAlign();
		// Allocate the render target memory
		surfaceAddresses[i] = m_GarlicAllocator.allocate(sizeAlign);
		if (!surfaceAddresses[i])
		{
			printf("Cannot allocate the render target memory\n");
			//return SCE_KERNEL_ERROR_ENOMEM;
			return false;
		}
		m_DisplayBuffers[i]->renderTarget.setAddresses(surfaceAddresses[i], 0, 0);

		m_DisplayBuffers[i]->displayIndex = i;
	}

	// Initialization the VideoOut buffer descriptor. The pixel format must
	// match with the render target data format, which in this case is
	// Gnm::kDataFormatB8G8R8A8UnormSrgb
	SceVideoOutBufferAttribute videoOutBufferAttribute;
	sceVideoOutSetBufferAttribute(
		&videoOutBufferAttribute,
		SCE_VIDEO_OUT_PIXEL_FORMAT_B8_G8_R8_A8_SRGB,
		SCE_VIDEO_OUT_TILING_MODE_TILE,
		SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
		m_BackBuffer->renderTarget.getWidth(),
		m_BackBuffer->renderTarget.getHeight(),
		m_BackBuffer->renderTarget.getPitch());

	// Register the buffers to the slot: [0..kDisplayBufferCount-1]
	ret = sceVideoOutRegisterBuffers(
		m_VideoOutHandle,
		0, // Start index
		surfaceAddresses,
		kDisplayBufferCount,
		&videoOutBufferAttribute);
	if (ret != SCE_OK)
	{
		printf("sceVideoOutRegisterBuffers failed: 0x%08X\n", ret);
		return false;
	}
	return true;
}

void NativeDevicePS4::FlipBackbuffer()
{
	// Rotate the display buffers
	m_BackBufferIndex = (m_BackBufferIndex + 1) % kDisplayBufferCount;
	m_BackBuffer = m_DisplayBuffers[m_BackBufferIndex];
}

bool NativeDevicePS4::InitSamplers()
{
	// Initialize the texture sampler
	m_Sampler.init();
	m_Sampler.setMipFilterMode(Gnm::kMipFilterModeNone);
	m_Sampler.setXyFilterMode(Gnm::kFilterModeBilinear, Gnm::kFilterModeBilinear);

	return true;
}

bool NativeDevicePS4::InitDepthBlend()
{
	static_assert(std::extent<decltype(m_BlendControl)>::value == std::extent<decltype(m_DepthStencilControl)>::value, "Do not change the array size. See RenderResourcePS4::Render");
	for (int k = 0; k < std::extent<decltype(m_BlendControl)>::value; ++k)
	{
		// Setup an additive blending mode
		m_BlendControl[k].init();
		m_BlendControl[k].setBlendEnable(k > 0);
		m_BlendControl[k].setColorEquation(
			Gnm::kBlendMultiplierSrcAlpha,
			Gnm::kBlendFuncAdd,
			Gnm::kBlendMultiplierOneMinusSrcAlpha);

		// Enable z-writes using a less comparison function
		m_DepthStencilControl[k].init();
		m_DepthStencilControl[k].setDepthControl(Gnm::kDepthControlZWriteEnable, DepthZDir == NativeDevice::FAR_IS_ZERO ? Gnm::kCompareFuncGreater : Gnm::kCompareFuncLess);
		m_DepthStencilControl[k].setDepthEnable(k > 0);
	}

	return true;
}

bool NativeDevicePS4::HasRenderDone(uint64_t frameNumber)
{
	if (m_FrameNumberGPU)
	{
		return (*m_FrameNumberGPU) >= frameNumber;
	}
	assert(false);
	return false;
}

void NativeDevicePS4::FreeGarlicMemory(unsigned int size, unsigned int alignment, void* ptr)
{
	if (size > 0 && ptr)
	{
		VRAMBlock newblock;
		newblock.Size = size;
		newblock.Alignment = alignment;
		newblock.GPUAddress = ptr;
		GarlicAllocatorPendingFree.push_back(newblock);
	}
}

void* NativeDevicePS4::AllocGarlicMemory(unsigned int sizeReqested, unsigned int alignment, unsigned int& sizeAllocated)
{
	for (auto itr = GarlicAllocatorFreeList.begin(), itend = GarlicAllocatorFreeList.end(); itr != itend; ++itr)
	{
		if (itr->Size >= sizeReqested &&
			itr->Alignment >= alignment &&
			(itr->Alignment % alignment) == 0)
		{
			sizeAllocated = itr->Size;
			void* result = itr->GPUAddress;
			GarlicAllocatorFreeList.erase(itr);
			return result;
		}
	}

	sizeAllocated = sizeReqested;
	void* ret = m_GarlicAllocator.allocate(sizeReqested, alignment);
	assert(ret != nullptr);
	return ret;
}

void NativeDevicePS4::FlushPendingFreeGarlicMemory()
{
	for (int k=0, kcount = GarlicAllocatorPendingFree.size(); k < kcount; ++k)
	{
		GarlicAllocatorFreeList.push_back(GarlicAllocatorPendingFree[k]);
	}
	GarlicAllocatorPendingFree.clear();

	std::sort(GarlicAllocatorFreeList.begin(), GarlicAllocatorFreeList.end(),
		[](const decltype(GarlicAllocatorFreeList)::value_type& a, const decltype(GarlicAllocatorFreeList)::value_type& b)
	{
		return a.Size < b.Size;
	});
}

bool FrameResourcePS4::Init(NativeDevicePS4* dvRes)
{
#if SCE_GNMX_ENABLE_GFX_LCUE
	// Calculate the size of the resource buffer for the given number of draw calls
	const uint32_t resourceBufferSizeInBytes =
		Gnmx::LightweightConstantUpdateEngine::computeResourceBufferSize(
			Gnmx::LightweightConstantUpdateEngine::kResourceBufferGraphics,
			kNumLcueBatches);

	// Allocate the LCUE resource buffer memory
	resourceBuffer = dvRes->m_GarlicAllocator.allocate(
		resourceBufferSizeInBytes, Gnm::kAlignmentOfBufferInBytes);

	if (!resourceBuffer)
	{
		printf("Cannot allocate the LCUE resource buffer memory\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	// Allocate the draw command buffer
	dcbBuffer = dvRes->m_OnionAllocator.allocate(
		kDcbSizeInBytes,
		Gnm::kAlignmentOfBufferInBytes);

	if (!dcbBuffer)
	{
		printf("Cannot allocate the draw command buffer memory\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	// Initialize the GfxContext used by this rendering context
	gfxContext.init(
		dcbBuffer,		// Draw command buffer address
		kDcbSizeInBytes,					// Draw command buffer size in bytes
		resourceBuffer,	// Resource buffer address
		resourceBufferSizeInBytes,			// Resource buffer address in bytes
		NULL);								// Global resource table

#else // SCE_GNMX_ENABLE_GFX_LCUE

	// Allocate the CUE heap memory
	cueHeap = dvRes->m_GarlicAllocator.allocate(
		Gnmx::ConstantUpdateEngine::computeHeapSize(kCueRingEntries),
		Gnm::kAlignmentOfBufferInBytes);

	if (!cueHeap)
	{
		printf("Cannot allocate the CUE heap memory\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	// Allocate the draw command buffer
	dcbBuffer = dvRes->m_OnionAllocator.allocate(
		kDcbSizeInBytes,
		Gnm::kAlignmentOfBufferInBytes);

	if (!dcbBuffer)
	{
		printf("Cannot allocate the draw command buffer memory\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	// Allocate the constants command buffer
	ccbBuffer = dvRes->m_OnionAllocator.allocate(
		kCcbSizeInBytes,
		Gnm::kAlignmentOfBufferInBytes);

	if (!ccbBuffer)
	{
		printf("Cannot allocate the constants command buffer memory\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	// Initialize the GfxContext used by this rendering context
	gfxContext.init(
		cueHeap,
		kCueRingEntries,
		dcbBuffer,
		kDcbSizeInBytes,
		ccbBuffer,
		kCcbSizeInBytes);
#endif	// SCE_GNMX_ENABLE_GFX_LCUE

	contextLabel = (volatile uint32_t*)dvRes->m_OnionAllocator.allocate(4, 8);
	if (!contextLabel)
	{
		printf("Cannot allocate a GPU label\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		return false;
	}

	contextLabel[0] = kRenderContextFree;

	return true;
}


void RenderPS4::Terminate()
{
	// Wait for the GPU to be idle before deallocating its resources
	for (uint32_t i = 0; i < kFrameResourceCount; ++i)
	{
		if (m_FrameResources[i]->contextLabel)
		{
			while (m_FrameResources[i]->contextLabel[0] != kRenderContextFree)
			{
				sceKernelUsleep(1000);
			}
		}
	}

	ReleaseOwnedResources();

	// Unregister the EOP event queue
	int ret;
	ret = Gnm::deleteEqEvent(m_NativeDeviceResource->m_EopEventQueue, Gnm::kEqEventGfxEop);
	if (ret != SCE_OK)
	{
		printf("Gnm::deleteEqEvent failed: 0x%08X\n", ret);
	}

	// Destroy the EOP event queue
	ret = sceKernelDeleteEqueue(m_NativeDeviceResource->m_EopEventQueue);
	if (ret != SCE_OK)
	{
		printf("sceKernelDeleteEqueue failed: 0x%08X\n", ret);
	}

	// Terminate the video output
	ret = sceVideoOutClose(m_NativeDeviceResource->m_VideoOutHandle);
	if (ret != SCE_OK)
	{
		printf("sceVideoOutClose failed: 0x%08X\n", ret);
	}

	// Releasing manually each allocated resource is not necessary as we are
	// terminating the linear allocators for ONION and GARLIC here.
	m_NativeDeviceResource->m_OnionAllocator.terminate();
	m_NativeDeviceResource->m_GarlicAllocator.terminate();
}

RenderResource* RenderPS4::CreateRenderResource()
{
	return new RenderResourcePS4();
}

RenderMeshResource* RenderPS4::CreateRenderMeshResource()
{
	return new RenderMeshResourcePS4();
}

RenderTextureResource* RenderPS4::CreateRenderTextureResource()
{
	return new RenderTextureResourcePS4();
}

RenderShaderResource* RenderPS4::CreateRenderShaderResource(RenderShaderResource::ShaderTypes _type)
{
	return new RenderShaderResourcePS4(_type);
}


void RenderPS4::CreateBuiltInDrawResources()
{
	m_DebugDrawLineResource = new RenderResourcePS4();
	m_DebugDrawLineResource->PrimitiveType = RenderResource::Primitive_LineList;
	m_DebugDrawLineResource->TextureResource = DefaultTexture;
	m_DebugDrawLineResource->MeshResource = CreateRenderMeshResource();
	m_DebugDrawLineResource->ShaderResource = CreateRenderShaderResource(RenderShaderResource::ShaderType_OpaqueBasic);

	m_DebugDrawLineResource->MeshResource->VerticesCPU.NumElements = MAX_NUM_DEBUG_DRAW_LINES * 2;
	m_DebugDrawLineResource->MeshResource->VerticesCPU.AllocBytes = sizeof(RenderMeshResource::MeshVertex) * m_DebugDrawLineResource->MeshResource->VerticesCPU.NumElements;
	m_DebugDrawLineResource->MeshResource->VerticesCPU.Data = (RenderMeshResource::MeshVertex*)malloc(m_DebugDrawLineResource->MeshResource->VerticesCPU.AllocBytes);

	m_DebugDrawLineResource->MeshResource->IndicesCPU.NumElements = MAX_NUM_DEBUG_DRAW_LINES * 2;
	m_DebugDrawLineResource->MeshResource->IndicesCPU.AllocBytes = sizeof(int) * m_DebugDrawLineResource->MeshResource->IndicesCPU.NumElements;
	m_DebugDrawLineResource->MeshResource->IndicesCPU.Data = (int*)malloc(m_DebugDrawLineResource->MeshResource->IndicesCPU.AllocBytes);
	m_DebugDrawLineResource->MeshResource->IsDynamicMesh = true;
	m_DebugDrawLineResource->PrimitiveType = RenderResource::Primitive_LineList;

	for (int k = 0; k < MAX_NUM_DEBUG_DRAW_LINES; ++k)
	{
		m_DebugDrawLineResource->MeshResource->IndicesCPU.Data[k] = k;
	}

	m_DebugDrawSphereResource = new RenderResourceInstancedPS4();
	m_DebugDrawSphereResource->TextureResource = DefaultTexture;
	m_DebugDrawSphereResource->MeshResource = CreateRenderMeshResource();
	m_DebugDrawSphereResource->MeshResource->LoadMesh_Sphere(0.5f, 16, 16);
	m_DebugDrawSphereResource->ShaderResource = CreateRenderShaderResource(RenderShaderResource::ShaderType_Instance3D);
	m_DebugDrawSphereResource->m_InstancedData3D.ReallocateCPUBuffer(1024);	// preserve but can grow again
	m_DebugDrawSphereResource->m_InstancedData2D.ReallocateCPUBuffer(1024);
	m_DebugDrawSphereResource->IsWireFrameMode = true;

	m_QuadMesh = new RenderMeshResourcePS4();
	m_QuadMesh->LoadMesh_Plane();
	m_QuadShader[QuadDraw2D] = new RenderShaderResourcePS4(RenderShaderResource::ShaderType_Instance2D);
	m_QuadShader[QuadDraw3D_DepthEnable] = new RenderShaderResourcePS4(RenderShaderResource::ShaderType_Instance3D);
	m_QuadShader[QuadDraw3D_DepthDisable] = new RenderShaderResourcePS4(RenderShaderResource::ShaderType_Instance3D);
}

void RenderPS4::SetViewProjection(float(&viewProj)[4][4])
{
	memcpy(this->ViewProj, viewProj, sizeof(viewProj));
}

void RenderPS4::SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture)
{
	m_DefaultFont = fontDesc;
	m_DefaultFontTexture = texture;
	m_DefaultFont->PostLoadFont(texture->Width, texture->Height);
}

void RenderPS4::DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB)
{
	DebugDraw::DrawArgs_Sphere s;
	memcpy(s.Pos, posXYZ, sizeof(s.Pos));
	memcpy(s.Scale, scaleXYZ, sizeof(s.Scale));
	memcpy(s.Color, colorRGB, sizeof(s.Color));
	DebugDrawSpheres.push_back(s);
}

void RenderPS4::DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB)
{
	DebugDraw::DrawArgs_Line li;
	memcpy(li.StartPos, pos0XYZ, sizeof(li.StartPos));
	memcpy(li.EndPos, pos1XYZ, sizeof(li.EndPos));
	memcpy(li.Color, colorRGB, sizeof(li.Color));
	DebugDrawLines.push_back(li);
}

int RenderPS4::SetupDebugDrawSphere()
{
	m_DebugDrawSphereResource->m_InstancedData3D.ReallocateCPUBuffer((int)DebugDrawSpheres.size());
	m_DebugDrawSphereResource->m_InstancedData3D.NumElementsCPU = (int)DebugDrawSpheres.size();

	for (int k = 0; k < DebugDrawSpheres.size(); ++k)
	{
		Matrix4 _xform = Matrix4::identity();
		// SRT
		_xform = _xform.scale(Vector3(DebugDrawSpheres[k].Scale[0], DebugDrawSpheres[k].Scale[1], DebugDrawSpheres[k].Scale[2]));
		_xform = _xform.translation(Vector3(DebugDrawSpheres[k].Pos[0], DebugDrawSpheres[k].Pos[1], DebugDrawSpheres[k].Pos[2]));
		m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].xform = ToMatrix4Unaligned(transpose(_xform)); // row major in shader;
		
		m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].uv_offset_scale = ToVector4Unaligned(Vector4(0, 0, 1, 1));
		m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].color = ToVector4Unaligned(Vector4(
					DebugDrawSpheres[k].Color[0], 
					DebugDrawSpheres[k].Color[1], 
					DebugDrawSpheres[k].Color[2], 
					1));
	}

	int num = DebugDrawSpheres.size();
	DebugDrawSpheres.clear();
	return num;
}

int RenderPS4::SetupDebugDrawLine()
{
	m_DebugDrawLineResource->MeshResource->VerticesCPU.Resize((int)DebugDrawLines.size() * 2);
	m_DebugDrawLineResource->MeshResource->IndicesCPU.Resize((int)DebugDrawLines.size() * 2);
	m_DebugDrawLineResource->MeshResource->VerticesCPU.NumElements = (int)DebugDrawLines.size() * 2;
	m_DebugDrawLineResource->MeshResource->IndicesCPU.NumElements = (int)DebugDrawLines.size() * 2;

	float _uv[2] = { 0, 0 };
	for (int k = 0; k < DebugDrawLines.size() * 2; k += 2)
	{
		float* _pos0 = DebugDrawLines[k / 2].StartPos;
		float* _pos1 = DebugDrawLines[k / 2].EndPos;
		float* _vtxcolor = DebugDrawLines[k / 2].Color;
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k].position, _pos0, sizeof(float) * 3);
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k].uv, _uv, sizeof(float) * 2);
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k].vtxcolor, _vtxcolor, sizeof(float) * 3);
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k + 1].position, _pos1, sizeof(float) * 3);
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k + 1].uv, _uv, sizeof(float) * 2);
		memcpy(m_DebugDrawLineResource->MeshResource->VerticesCPU.Data[k + 1].vtxcolor, _vtxcolor, sizeof(float) * 3);

		m_DebugDrawLineResource->MeshResource->IndicesCPU.Data[k] = k;
		m_DebugDrawLineResource->MeshResource->IndicesCPU.Data[k + 1] = k + 1;
	}

	int num = DebugDrawLines.size();
	DebugDrawLines.clear();
	return num;
}

void RenderPS4::FlushEndFrameDraws(sce::Gnmx::GnmxGfxContext& gfxc)
{
	FlushQuadDraws(gfxc);

	int numSpheres = SetupDebugDrawSphere();
	if (numSpheres > 0)
	{
		m_DebugDrawSphereResource->PrepareRender(this);
		RenderBufferUtil::CopyShaderMatrix(m_DebugDrawSphereResource->ConstBufferCPU.WorldViewProj, this->ViewProj);
		m_DebugDrawSphereResource->Render(this);
	}
	

	int numLines = SetupDebugDrawLine();
	if (numLines > 0)
	{
		m_DebugDrawLineResource->PrepareRender(this);
		RenderBufferUtil::CopyShaderMatrix(m_DebugDrawLineResource->ConstBufferCPU.WorldViewProj, this->ViewProj);
		m_DebugDrawLineResource->Render(this);
	}
}

RenderResourceInstancedPS4* RenderPS4::GetOrCreateNextRenderResourceInstanced(QuadDrawType drawType, RenderTextureResource* texture)
{
	RenderResourceInstancedPS4* res = nullptr;
	for (int k = 0; k < m_QuadDraws[drawType].size(); ++k)
	{
		if (m_QuadDraws[drawType][k]->TextureResource == texture)	// combine batch if texture is the same
		{
			res = m_QuadDraws[drawType][k].get();
			break;
		}
	}
	if (res == nullptr)
	{
		m_QuadDraws[drawType].emplace_back(new RenderResourceInstancedPS4());
		res = m_QuadDraws[drawType].back().get();
		res->MeshResource = m_QuadMesh;
		res->ShaderResource = m_QuadShader[drawType];
		res->TextureResource = texture;
		switch (drawType)
		{
		case QuadDraw3D_DepthEnable:
		case QuadDraw3D_DepthDisable:
			res->EnableDepthTest = drawType == QuadDraw3D_DepthEnable;
			break;
		case QuadDraw2D:
			res->EnableDepthTest = DRAWQUAD2D_DEFAULT_DEPTH_ENABLE;
			break;
		default:
			res->EnableDepthTest = true;
		}
	}

	return res;
}

void RenderPS4::DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture, bool isDepthEnable)
{
	RenderResourceInstancedPS4* res = GetOrCreateNextRenderResourceInstanced(isDepthEnable ? QuadDraw3D_DepthEnable : QuadDraw3D_DepthDisable, texture);
	res->m_InstancedData3D.TryReallocateCPUBuffer();
	
	PerInstanceData3D& instanceData = res->m_InstancedData3D.DataCPU[res->m_InstancedData3D.NumElementsCPU];
	Matrix4 _xform;
	memcpy(&_xform, worldTransform, sizeof(worldTransform));
	instanceData.xform = ToMatrix4Unaligned(transpose(_xform)); // row major in shader;
	Vector4 uvoffsetscale;
	memcpy(&uvoffsetscale, uvOffsetScale, sizeof(uvOffsetScale));
	instanceData.uv_offset_scale = ToVector4Unaligned(uvoffsetscale);
	Vector4 veccolor;
	memcpy(&veccolor, color, sizeof(color));
	instanceData.color = ToVector4Unaligned(veccolor);

	res->m_InstancedData3D.NumElementsCPU++;
}

void RenderPS4::DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture)
{
	RenderResourceInstancedPS4* res = GetOrCreateNextRenderResourceInstanced(QuadDraw2D, texture);
	res->m_InstancedData2D.TryReallocateCPUBuffer();

	PerInstanceData2D& instanceData = res->m_InstancedData2D.DataCPU[res->m_InstancedData2D.NumElementsCPU];
	Matrix4 _xform;
	memcpy(&_xform, clipTransform, sizeof(clipTransform));
	instanceData.xform = ToMatrix4Unaligned(transpose(_xform)); // row major in shader; // use 4x4 as storage for using shader built-in math functions
	Vector4 uvoffsetscale;
	memcpy(&uvoffsetscale, uvOffsetScale, sizeof(uvOffsetScale));
	instanceData.uv_offset_scale = ToVector4Unaligned(uvoffsetscale);
	Vector4 veccolor;
	memcpy(&veccolor, color, sizeof(color));
	instanceData.color = ToVector4Unaligned(veccolor);

	res->m_InstancedData2D.NumElementsCPU++;
}

void RenderPS4::FlushQuadDraws(sce::Gnmx::GnmxGfxContext& gfxc)
{
	for (int q = 0; q < NumQuadDrawTypes; ++q)
	{
		for (int k = 0; k < m_QuadDraws[q].size(); ++k)
		{
			m_QuadDraws[q][k]->PrepareRender(this);
			RenderBufferUtil::CopyShaderMatrix(m_QuadDraws[q][k]->ConstBufferCPU.WorldViewProj, this->ViewProj);
			m_QuadDraws[q][k]->Render(this);
			m_QuadDraws[q][k]->ExitRender(this, false);	// no mesh/texture/shader will be deleted. Instance data is in garlic pending free
		}
		m_QuadDraws[q].clear();
	}
}

struct DrawTextScratchPad
{
	float CharQuadLocalTransform[4][4];	// the xform from origin of the sentence to character quad
	float CharQuadWorldTransform[4][4];	// the xform from origin of 3D world to character quad
	float UVOffsetScale[4];
};
static DrawTextScratchPad* GetDrawTextScratchPad()
{
	static thread_local DrawTextScratchPad* s_DrawTextScratchpad = new DrawTextScratchPad();
	return s_DrawTextScratchpad;
}


void RenderPS4::DrawText3D(const char* text, float(&worldTransform)[4][4], float(&color)[4], float fontScale, bool isDepthEnable)
{
	int length = (int)strlen(text);
	assert(length < 1024);	// null terminated ?

	DrawTextScratchPad* scratchPad = GetDrawTextScratchPad();

	const float quadWidth = 1.0f;
	BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
	for (int k = 0; k < length; ++k)
	{
		m_DefaultFont->BuildInstanceData(ctx, text[k], scratchPad->CharQuadLocalTransform, scratchPad->UVOffsetScale);
		RenderBufferUtil::MulMatrix((float*)&(scratchPad->CharQuadLocalTransform),
			(float*)&worldTransform,
			(float*)&(scratchPad->CharQuadWorldTransform));
		DrawQuad3D(scratchPad->CharQuadWorldTransform, scratchPad->UVOffsetScale, color, m_DefaultFontTexture, isDepthEnable);
	}
}



void RenderPS4::DrawText2D(const char* text, float(&clipTransform)[4][4], float(&color)[4], float fontScale)
{
	int length = (int)strlen(text);
	assert(length < 1024);	// null terminated ?

	DrawTextScratchPad* scratchPad = GetDrawTextScratchPad();

	const float quadWidth = 1.0f;
	BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
	for (int k = 0; k < length; ++k)
	{
		m_DefaultFont->BuildInstanceData(ctx, text[k], scratchPad->CharQuadLocalTransform, scratchPad->UVOffsetScale);
		RenderBufferUtil::MulMatrix((float*)&(scratchPad->CharQuadLocalTransform),
			(float*)&clipTransform,
			(float*)&(scratchPad->CharQuadWorldTransform));
		DrawQuad2D(scratchPad->CharQuadWorldTransform, scratchPad->UVOffsetScale, color, m_DefaultFontTexture);
	}
}

float RenderPS4::GetTextTotalWidth(const char* text, float fontScale)
{
	const float quadWidth = 1.0f;
	return m_DefaultFont->GetTextTotalWidth(text, fontScale, quadWidth);
}

#endif