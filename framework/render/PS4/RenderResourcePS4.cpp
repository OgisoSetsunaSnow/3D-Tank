#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <stdio.h>
#include <stdlib.h>
#include <scebase.h>
#include <kernel.h>
#include <vectormath.h>
#include <assert.h>
#include <chrono>
#include <thread>
#include "render/PS4/api_gnm/toolkit/toolkit.h"
#include "render/PS4/common/allocator.h"
#include "render/PS4/common/shader_loader.h"

#include "assets/ps4_shader/std_cbuffer.h"

#include "render/RenderInterface.h"
#include "render/PS4/RenderPS4.h"
#include "render/PS4/RenderResourcePS4.h"

using namespace sce;
#define PTR_OFFSET(ptr,size) (void*)((size_t)ptr+size)

namespace RenderBufferUtil
{
	void CopyShaderMatrix(float(&dst)[4][4], float(&src)[4][4])
	{
		TransposeMatrix((float*)dst, (float*)src);
	}
}

bool RenderMeshResourcePS4::SetupVB(NativeDevicePS4* dvRes)
{
	bool isDirty = false;
	if (m_VertexBufferGPUBytes < VerticesCPU.GetDataBytes())
	{
		assert(m_VertexBufferGPUBytes == 0 || IsDynamicMesh);
		dvRes->FreeGarlicMemory(m_VertexBufferGPUBytes, 
								Gnm::kAlignmentOfBufferInBytes, 
								m_VertexBufferGPUAddress);  // TODO truely release garlic memory
		m_VertexBufferGPUAddress = static_cast<RenderMeshResource::MeshVertex*>(dvRes->AllocGarlicMemory(
								VerticesCPU.GetDataBytes(), 
								Gnm::kAlignmentOfBufferInBytes,
								m_VertexBufferGPUBytes));
		if (!m_VertexBufferGPUAddress)
		{
			printf("Cannot allocate vertex data\n");
			assert(false);
			//return SCE_KERNEL_ERROR_ENOMEM;
			return false;
		}

		// Initialize the vertex buffers pointing to each vertex element
		int numVtx = VerticesCPU.GetDataBytes() / sizeof(VerticesCPU.Data[0]);
		int stride = sizeof(VerticesCPU.Data[0]);

		m_VertexBuffersGPU[kVertexPosition].initAsVertexBuffer(&m_VertexBufferGPUAddress->position, Gnm::kDataFormatR32G32B32Float, stride, numVtx);
		m_VertexBuffersGPU[kVertexColor].initAsVertexBuffer(&m_VertexBufferGPUAddress->vtxcolor, Gnm::kDataFormatR32G32B32Float, stride, numVtx);
		m_VertexBuffersGPU[kVertexUv].initAsVertexBuffer(&m_VertexBufferGPUAddress->uv, Gnm::kDataFormatR32G32Float, stride, numVtx);
		isDirty = true;
	}

	if (m_IndexBufferGPUBytes < IndicesCPU.GetDataBytes())
	{
		assert(m_IndexBufferGPUBytes == 0 || IsDynamicMesh);
		dvRes->FreeGarlicMemory(m_IndexBufferGPUBytes,
			Gnm::kAlignmentOfBufferInBytes,
			m_IndexBufferGPUAddress);  // TODO truely release garlic memory

		m_IndexBufferGPUAddress = static_cast<int*>(dvRes->AllocGarlicMemory(
			IndicesCPU.GetDataBytes(), 
			Gnm::kAlignmentOfBufferInBytes,
			m_IndexBufferGPUBytes));
		if (!m_IndexBufferGPUAddress)
		{
			printf("Cannot allocate index data\n");
			assert(false);
			//return SCE_KERNEL_ERROR_ENOMEM;
			return false;
		}
		isDirty = true;
	}

	if (isDirty || IsDynamicMesh)
	{
		// Copy the vertex/index data onto the GPU mapped memory
		memcpy(m_VertexBufferGPUAddress, VerticesCPU.Data, VerticesCPU.GetDataBytes());
		memcpy(m_IndexBufferGPUAddress, IndicesCPU.Data, IndicesCPU.GetDataBytes());
	}

	return true;
}

void RenderMeshResourcePS4::Release(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	rndps4->m_NativeDeviceResource->FreeGarlicMemory(m_VertexBufferGPUBytes, Gnm::kAlignmentOfBufferInBytes, m_VertexBufferGPUAddress);
	rndps4->m_NativeDeviceResource->FreeGarlicMemory(m_IndexBufferGPUBytes, Gnm::kAlignmentOfBufferInBytes, m_IndexBufferGPUAddress);
}

#define COPY_TEXTURE_BY_TILE

bool RenderTextureResourcePS4::SetupTexture(NativeDevicePS4* dvRes)
{
	if (TextureGPU.getBaseAddress() != nullptr)
	{
		assert(DebugTextureCPUAddress == RGBATextureDataCPU->PixelData);	// do not use RenderTextureResource for another texture
		assert(DebugTextureWidth == Width && DebugTextureHeight == Height);	// do not use RenderTextureResource for another texture
		return true;
	}

	sce::Gnm::TileMode tileMode = Gnm::kTileModeDisplay_LinearAligned;
#ifdef COPY_TEXTURE_BY_TILE
	int32_t tileRet = sce::GpuAddress::computeSurfaceTileMode(dvRes->m_GpuMode, &tileMode, sce::GpuAddress::kSurfaceTypeTextureFlat, sce::Gnm::kDataFormatR8G8B8A8UnormSrgb, 1);
	if (tileRet != SCE_GNM_OK)
	{
		assert(false);
		return false;
	}
#endif
	// Initialize a Gnm::Texture object
	{
		Gnm::TextureSpec spec;
		spec.init();
		spec.m_textureType = Gnm::kTextureType2d;
		spec.m_width = Width;
		spec.m_height = Height;
		spec.m_depth = 1;
		spec.m_pitch = 0;
		spec.m_numMipLevels = 1;
		spec.m_numSlices = 1;
		spec.m_format = Gnm::kDataFormatR8G8B8A8UnormSrgb;
		spec.m_tileModeHint = Gnm::kTileModeDisplay_LinearAligned;
		spec.m_minGpuMode = dvRes->m_GpuMode;
		spec.m_numFragments = Gnm::kNumFragments1;
		int32_t status = TextureGPU.init(&spec);

		if (status != SCE_GNM_OK)
		{
			assert(false);
			return false;
		}
	}

	sce::Gnm::SizeAlign textureGpuSize = TextureGPU.getSizeAlign();
	// Allocate the texture data using the alignment returned by initAs2d
	unsigned int textureGpuSizeAllocated = 0;
	void* stagingBuffer = dvRes->AllocGarlicMemory(textureGpuSize.m_size, textureGpuSize.m_align, textureGpuSizeAllocated);
	if (!stagingBuffer)
	{
		printf("Cannot allocate the texture data\n");
		//return SCE_KERNEL_ERROR_ENOMEM;
		assert(false);
		return false;
	}

#ifdef COPY_TEXTURE_BY_TILE
	sce::GpuAddress::TilingParameters tp;
	if (tp.initFromTexture(&TextureGPU, 0, 0) != SCE_GNM_OK || 
		sce::GpuAddress::tileSurface(stagingBuffer, RGBATextureDataCPU->PixelData, &tp) != SCE_GNM_OK)
	{
		printf("Gnm: Error when copy texture by tile. See it return value for error code.\n");
		assert(false);
		return false;
	}
#else
	unsigned int pitch = TextureGPU.getPitch();
	if (pitch == Width)
	{
		memcpy(stagingBuffer, RGBATextureDataCPU->PixelData, Width * Height * RGBAPixel::GetBytesPerPixel());
	}
	else if(pitch > Width)
	{
		for (int y=0; y< Height; ++y)
		{
			memcpy(PTR_OFFSET(stagingBuffer, y * pitch * RGBAPixel::GetBytesPerPixel()),
					PTR_OFFSET(RGBATextureDataCPU->PixelData, y * Width * RGBAPixel::GetBytesPerPixel()),
					Width * RGBAPixel::GetBytesPerPixel());
		}
	}
	else
	{
		assert(false);	// a row of pixel can't fit a pitch
	}
#endif

	// Set the base data address in the texture object
	TextureGPU.setBaseAddress(stagingBuffer);
	DebugTextureCPUAddress = RGBATextureDataCPU->PixelData;
	DebugTextureWidth = Width;
	DebugTextureHeight = Height;
	return true;
}

void RenderTextureResourcePS4::Release(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	rndps4->m_NativeDeviceResource->FreeGarlicMemory(TextureGPU.getSizeAlign().m_size, TextureGPU.getSizeAlign().m_align, TextureGPU.getBaseAddress());
}


void RenderResourcePS4::PrepareRender(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	RenderMeshResourcePS4* meshResPS4 = (RenderMeshResourcePS4*)MeshResource;
	RenderTextureResourcePS4* texResPS4 = (RenderTextureResourcePS4*)TextureResource;
	RenderShaderResourcePS4* shaderResPS4 = (RenderShaderResourcePS4*)ShaderResource;
	assert(shaderResPS4 != nullptr);
	shaderResPS4->SetupShader(rndps4->m_NativeDeviceResource);
	assert(meshResPS4 != nullptr);
	meshResPS4->SetupVB(rndps4->m_NativeDeviceResource);
	assert(texResPS4 != nullptr);
	texResPS4->SetupTexture(rndps4->m_NativeDeviceResource);
}

static sce::Gnm::PrimitiveType GetTopologyGnm(RenderResource::PrimitiveTypes _type)
{
	switch (_type)
	{
	case RenderResource::Primitive_TriangleList: return sce::Gnm::PrimitiveType::kPrimitiveTypeTriList;
	case RenderResource::Primitive_LineList: return sce::Gnm::PrimitiveType::kPrimitiveTypeLineList;
	default: assert(false);
	}
	return sce::Gnm::PrimitiveType::kPrimitiveTypeTriList;
}

void RenderResourcePS4::SetupNumInstances(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	Gnmx::GnmxGfxContext& gfxc = rndps4->m_FrameResource->gfxContext;
	gfxc.setNumInstances(1);
}

void RenderResourcePS4::Render(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	RenderMeshResourcePS4* meshResPS4 = (RenderMeshResourcePS4*)MeshResource;
	RenderTextureResourcePS4* texResPS4 = (RenderTextureResourcePS4*)TextureResource;
	RenderShaderResourcePS4* shd = (RenderShaderResourcePS4*)ShaderResource;
	Gnmx::GnmxGfxContext& gfxc = rndps4->m_FrameResource->gfxContext;

	// Cull clock-wise backfaces
	Gnm::PrimitiveSetup primSetupReg;
	primSetupReg.init();
	primSetupReg.setCullFace(Gnm::kPrimitiveSetupCullFaceBack);
	primSetupReg.setFrontFace(meshResPS4->CullMode == RenderMeshResource::CCW_FRONT ? Gnm::kPrimitiveSetupFrontFaceCcw : Gnm::kPrimitiveSetupFrontFaceCw);
	primSetupReg.setPolygonMode(IsWireFrameMode ? Gnm::kPrimitiveSetupPolygonModeLine : Gnm::kPrimitiveSetupPolygonModeFill, Gnm::kPrimitiveSetupPolygonModeFill);
	gfxc.setPrimitiveSetup(primSetupReg);

	gfxc.setBlendControl(0, rndps4->m_NativeDeviceResource->m_BlendControl[this->EnableAlphaBlend ? 1 : 0]);

	gfxc.setDepthStencilControl(rndps4->m_NativeDeviceResource->m_DepthStencilControl[this->EnableDepthTest ? 1 : 0]);

	// Setup the output color mask
	gfxc.setRenderTargetMask(0xF);

	// Activate the VS and PS shader stages
	gfxc.setActiveShaderStages(Gnm::kActiveShaderStagesVsPs);
	gfxc.setVsShader(shd->m_VertexShader, shd->m_ShaderModifier, shd->m_FetchShaderMemory, &shd->m_VSInputOffsetsCache);
	gfxc.setPsShader(shd->m_PixelShader, &shd->m_PSInputOffsetsCache);

	// Setup the vertex buffer used by the ES stage (vertex shader)
	// Note that the setXxx methods of GfxContext which are used to set
	// shader resources (e.g.: V#, T#, S#, ...) map directly on the
	// Constants Update Engine. These methods do not directly produce PM4
	// packets in the command buffer. The CUE gathers all the resource
	// definitions and creates a set of PM4 packets later on in the
	// gfxc.drawXxx method.
	gfxc.setVertexBuffers(Gnm::kShaderStageVs, 0, kVertexElemCount, meshResPS4->m_VertexBuffersGPU);

	// Setup the texture and its sampler on the PS stage
	gfxc.setTextures(Gnm::kShaderStagePs, 0, 1, &texResPS4->TextureGPU);
	gfxc.setSamplers(Gnm::kShaderStagePs, 0, 1, &rndps4->m_NativeDeviceResource->m_Sampler);

	// Allocate the vertex shader constants from the command buffer
	ConstantBuffer* constBufferStaging = static_cast<ConstantBuffer*>(
		gfxc.allocateFromCommandBuffer(sizeof(ConstantBuffer), Gnm::kEmbeddedDataAlignment4));

	// Initialize the vertex shader constants
	if (constBufferStaging)
	{
		memcpy(constBufferStaging, &ConstBufferCPU, sizeof(ConstantBuffer));
		Gnm::Buffer constBufferObj;
		constBufferObj.initAsConstantBuffer(constBufferStaging, sizeof(ConstantBuffer));
		gfxc.setConstantBuffers(Gnm::kShaderStageVs, 0, 1, &constBufferObj);
	}
	else
	{
		printf("Cannot allocate vertex shader constants\n");
	}

	// Submit a draw call
	gfxc.setPrimitiveType(GetTopologyGnm(PrimitiveType));
	gfxc.setIndexSize(Gnm::kIndexSize32);
	SetupNumInstances(rndps4);
	gfxc.drawIndex(meshResPS4->IndicesCPU.NumElements, meshResPS4->m_IndexBufferGPUAddress);

	m_FrameNumberToRender = rndps4->m_NativeDeviceResource->m_FrameNumberCPU;
}

void RenderResourcePS4::WaitForRenderDone(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	while (!rndps4->m_NativeDeviceResource->HasRenderDone(m_FrameNumberToRender))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void RenderResourcePS4::ExitRender(RenderInterface* rnd, bool waitForRenderDone)
{
	if(waitForRenderDone)
		WaitForRenderDone(rnd);
}














//--------------------------------------------------------------------------------
void RenderShaderResourcePS4::ReleaseGPUResource(RenderInterface* rnd)
{
#if 0	// linear allocator cannot release a block in the middle
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	if (m_VertexShader)
	{
		rndps4->m_NativeDeviceResource->m_ToolkitAllocators.release(m_VertexShader);
		m_VertexShader = nullptr;
	}
	if (m_PixelShader)
	{
		rndps4->m_NativeDeviceResource->m_ToolkitAllocators.release(m_PixelShader);
		m_PixelShader = nullptr;
	}
	if (m_FetchShaderMemory)
	{
		rndps4->m_NativeDeviceResource->m_GarlicAllocator.release(m_PixelShader);
		m_FetchShaderMemory = nullptr;
	}
#endif
}

void RenderShaderResourcePS4::SetupShader(NativeDevicePS4* dvRes)
{
	if (m_VertexShader && m_PixelShader)
		return;

	switch (ShaderType)
	{
	case RenderShaderResource::ShaderType_OpaqueBasic:
		SetupOpaqueBasicShader(dvRes);
		break;
	case RenderShaderResource::ShaderType_Instance3D:
		SetupInstance3DShader(dvRes);
		break;
	case RenderShaderResource::ShaderType_Instance2D:
		SetupInstance2DShader(dvRes);
		break;
	default:
		assert(false);
	}
}

void RenderShaderResourcePS4::SetupOpaqueBasicShader(NativeDevicePS4* dvRes)
{
	const char* vs = "/app0/framework/assets/ps4_shader/opaque_basic_vv.sb";
	const char* ps = "/app0/framework/assets/ps4_shader/opaque_basic_p.sb";
	Gnm::FetchShaderInstancingMode* instancingMode = nullptr;
	SetupShaderInternal(vs, ps, dvRes, instancingMode, 0);
}

void RenderShaderResourcePS4::SetupInstance3DShader(NativeDevicePS4* dvRes)
{
	const char* vs = "/app0/framework/assets/ps4_shader/instance_3d_vv.sb";
	const char* ps = "/app0/framework/assets/ps4_shader/instance_3d_p.sb";

#if 0
	// instacing mode definition on all vertex attr if instance data is bind to vertex buffer slots and use non-system semantics 
	// see bubbleRender.m_perInstanceBuffer in SDK sample
	Gnm::FetchShaderInstancingMode instancingMode[] =
	{
		Gnm::kFetchShaderUseVertexIndex,			 // Position,				no instancing
		Gnm::kFetchShaderUseVertexIndex,			 // Color,					no instancing
		Gnm::kFetchShaderUseVertexIndex,			 // TextureUV,				no instancing
		Gnm::kFetchShaderUseInstanceId,				 // UserInstanceData(vec4),	instancing using Id
	};
	int numInstancingMode = sizeof(instancingMode) / sizeof(instancingMode[0]);
#else
	// use SRV buffer. No instancing data in vertex buffer.
	Gnm::FetchShaderInstancingMode* instancingMode = nullptr;
	int numInstancingMode = 0;
#endif
	SetupShaderInternal(vs, ps, dvRes, instancingMode, numInstancingMode);
}

void RenderShaderResourcePS4::SetupInstance2DShader(NativeDevicePS4* dvRes)
{
	const char* vs = "/app0/framework/assets/ps4_shader/instance_2d_vv.sb";
	const char* ps = "/app0/framework/assets/ps4_shader/instance_2d_p.sb";

#if 0
	// instacing mode definition on all vertex attr if instance data is bind to vertex buffer slots and use non-system semantics 
	// see bubbleRender.m_perInstanceBuffer in SDK sample
	Gnm::FetchShaderInstancingMode instancingMode[] =
	{
		Gnm::kFetchShaderUseVertexIndex,			 // Position,				no instancing
		Gnm::kFetchShaderUseVertexIndex,			 // Color,					no instancing
		Gnm::kFetchShaderUseVertexIndex,			 // TextureUV,				no instancing
		Gnm::kFetchShaderUseInstanceId,				 // UserInstanceData(vec4),	instancing using Id
	};
	int numInstancingMode = sizeof(instancingMode) / sizeof(instancingMode[0]);
#else
	// use SRV buffer. No instancing data in vertex buffer.
	Gnm::FetchShaderInstancingMode* instancingMode = nullptr;
	int numInstancingMode = 0;
#endif
	SetupShaderInternal(vs, ps, dvRes, instancingMode, numInstancingMode);
}


void RenderShaderResourcePS4::SetupShaderInternal(const char* vs_path, const char* ps_path, NativeDevicePS4* dvRes, sce::Gnm::FetchShaderInstancingMode* instancingModePerAttr, int numInstanceAttr)
{
	if (m_VertexShader && m_PixelShader)
		return;

	// Load the shader binaries from disk
	m_VertexShader = loadShaderFromFile<Gnmx::VsShader>(vs_path, dvRes->m_ToolkitAllocators);
	m_PixelShader = loadShaderFromFile<Gnmx::PsShader>(ps_path, dvRes->m_ToolkitAllocators);
	if (!m_VertexShader || !m_PixelShader)
	{
		printf("Cannot load the shaders\n");
		assert(false);
		//return SCE_KERNEL_ERROR_EIO;
		return;
	}

	// Allocate the memory for the fetch shader
	m_FetchShaderMemory = dvRes->m_GarlicAllocator.allocate(
		Gnmx::computeVsFetchShaderSize(m_VertexShader),
		Gnm::kAlignmentOfFetchShaderInBytes);
	if (!m_FetchShaderMemory)
	{
		printf("Cannot allocate the fetch shader memory\n");
		assert(false);
		//return SCE_KERNEL_ERROR_ENOMEM;
		return;
	}

	// Generate the fetch shader for the VS stage
	Gnmx::generateVsFetchShader(m_FetchShaderMemory, &m_ShaderModifier, m_VertexShader, instancingModePerAttr, numInstanceAttr);

	// Generate the shader input caches.
	// Using a pre-calculated shader input cache is optional with CUE but it
	// normally reduces the CPU time necessary to build the command buffers.

	Gnmx::generateInputOffsetsCache(&m_VSInputOffsetsCache, Gnm::kShaderStageVs, m_VertexShader);
	Gnmx::generateInputOffsetsCache(&m_PSInputOffsetsCache, Gnm::kShaderStagePs, m_PixelShader);
}


template<typename ELEMENT_TYPE>
void RenderResourceInstancedPS4::ReallocateInstanceBufferGPU(RenderPS4* rnd, RenderResourceInstancedPS4::InstanceBuffer<ELEMENT_TYPE>& _buffer)
{
	if (_buffer.AllocBytesGPU < _buffer.GetDataBytesInUse())
	{
		rnd->m_NativeDeviceResource->FreeGarlicMemory(_buffer.AllocBytesGPU, Gnm::kAlignmentOfBufferInBytes, _buffer.DataGPUAddress);
		_buffer.DataGPUAddress = (ELEMENT_TYPE*)rnd->m_NativeDeviceResource->AllocGarlicMemory(_buffer.AllocBytesCPU, Gnm::kAlignmentOfBufferInBytes, _buffer.AllocBytesGPU);
		_buffer.DataGPU.initAsRegularBuffer(_buffer.DataGPUAddress, _buffer.GetStride(), _buffer.NumElementsCPU);
	}
}

template<typename ELEMENT_TYPE>
void RenderResourceInstancedPS4::UpdateInstanceBufferGPU(RenderResourceInstancedPS4::InstanceBuffer<ELEMENT_TYPE>& _buffer)
{
	memcpy(_buffer.DataGPUAddress, _buffer.DataCPU, _buffer.GetDataBytesInUse());	// unified memory architecture
}

void RenderResourceInstancedPS4::SetupNumInstances(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	Gnmx::GnmxGfxContext& gfxc = rndps4->m_FrameResource->gfxContext;
	assert(m_InstancedData3D.NumElementsCPU == 0 || m_InstancedData2D.NumElementsCPU == 0);
	gfxc.setNumInstances(m_InstancedData3D.NumElementsCPU > 0 ? m_InstancedData3D.NumElementsCPU : m_InstancedData2D.NumElementsCPU);
}


void RenderResourceInstancedPS4::Render(RenderInterface* rnd)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	Gnmx::GnmxGfxContext& gfxc = rndps4->m_FrameResource->gfxContext;

	if (m_InstancedData3D.NumElementsCPU > 0)
	{
		ReallocateInstanceBufferGPU(rndps4, m_InstancedData3D);
		UpdateInstanceBufferGPU(m_InstancedData3D);
		// see instance_buffer.h, share the same slot pool with texture
		gfxc.setBuffers(Gnm::kShaderStageVs, RenderPS4::SRVTable_InstanceBuffer3D_t4, 1, &(m_InstancedData3D.DataGPU));
	}

	if (m_InstancedData2D.NumElementsCPU > 0)
	{
		ReallocateInstanceBufferGPU(rndps4, m_InstancedData2D);
		UpdateInstanceBufferGPU(m_InstancedData2D);
		// see instance_buffer.h, share the same slot pool with texture
		gfxc.setBuffers(Gnm::kShaderStageVs, RenderPS4::SRVTable_InstanceBuffer2D_t5, 1, &(m_InstancedData2D.DataGPU));
	}

	RenderResourcePS4::Render(rnd);
}

void RenderResourceInstancedPS4::ExitRender(RenderInterface* rnd, bool waitForRenderDone)
{
	RenderPS4* rndps4 = (RenderPS4*)rnd;
	if (m_InstancedData3D.DataGPUAddress)
	{
		rndps4->m_NativeDeviceResource->FreeGarlicMemory(m_InstancedData3D.AllocBytesGPU, Gnm::kAlignmentOfBufferInBytes, m_InstancedData3D.DataGPUAddress);
	}
	if (m_InstancedData2D.DataGPUAddress)
	{
		rndps4->m_NativeDeviceResource->FreeGarlicMemory(m_InstancedData2D.AllocBytesGPU, Gnm::kAlignmentOfBufferInBytes, m_InstancedData2D.DataGPUAddress);
	}
	RenderResourcePS4::ExitRender(rnd, waitForRenderDone);
}

#endif