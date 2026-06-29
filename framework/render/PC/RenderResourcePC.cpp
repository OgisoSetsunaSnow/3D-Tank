#include "render/PC/RenderResourcePC.h"

#if VTS_PLATFORM_WINDOWS
#include "render/PC/ImageIO.h"
#include "assets/embedded/RubyTexture.h"
#include "assets/pc_shader/Shaders.h"

#include "d3dx12.h"
#include <d3dcompiler.h>
#include <cmath>
#include <assert.h>
#include <stdio.h>
#include "render/PC/RenderPC.h"
#include "render/PC/RenderPCNative.h"
using namespace Microsoft::WRL;

namespace RenderBufferUtil
{
	void CopyShaderMatrix(float(&dst)[4][4], float(&src)[4][4])
	{
		TransposeMatrix((float*)dst, (float*)src);
	}
}

bool RenderMeshResourcePC::RecreateMeshBuffers(RenderPC* rnd)
{
	auto& device_ = rnd->m_NativeDevice->device_;
	bool isDirty = false;
	const unsigned int uploadBufferSize = VerticesCPU.GetDataBytes() + IndicesCPU.GetDataBytes();
	if (StagingBufferGPUBytes < uploadBufferSize)
	{
		rnd->EnsureAliveThisFrame(StagingBufferGPU);
		// Create upload buffer on CPU
		
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		device_->CreateCommittedResource(&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&StagingBufferGPU));
		StagingBufferGPUBytes = uploadBufferSize;
		isDirty = true;
	}
	

	// Create vertex & index buffer on the GPU
	// HEAP_TYPE_DEFAULT is on GPU, we also initialize with COPY_DEST state
	// so we don't have to transition into this before copying into them
	static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	if (VertexBufferGPUBytes < VerticesCPU.GetDataBytes())
	{
		rnd->EnsureAliveThisFrame(VertexBufferGPU);
		const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VerticesCPU.GetDataBytes());
        device_->CreateCommittedResource(&defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&VertexBufferGPU));
		VertexBufferGPUBytes = VerticesCPU.GetDataBytes();
		VertexBufferView.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		VertexBufferView.SizeInBytes = VerticesCPU.GetDataBytes();
		VertexBufferView.StrideInBytes = sizeof(decltype(*VerticesCPU.Data));
		isDirty = true;
	}

	if (IndexBufferGPUBytes < IndicesCPU.GetDataBytes())
	{
		rnd->EnsureAliveThisFrame(IndexBufferGPU);
		const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(IndicesCPU.GetDataBytes());
        device_->CreateCommittedResource(&defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &indexBufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&IndexBufferGPU));
		IndexBufferGPUBytes = IndicesCPU.GetDataBytes();
		IndexBufferView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		IndexBufferView.SizeInBytes = IndicesCPU.GetDataBytes();
		assert(sizeof(decltype(*IndicesCPU.Data)) == sizeof(int));
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		isDirty = true;
	}

	return isDirty;
}

void RenderMeshResourcePC::SetupMeshBuffers(RenderPC* rnd, ID3D12GraphicsCommandList* uploadCommandList)
{
	bool isDirty = RecreateMeshBuffers(rnd);

	if (isDirty || IsDynamicMesh)
	{
		// Copy data on CPU into the upload buffer
		void* p;
		StagingBufferGPU->Map(0, nullptr, &p);
		::memcpy(p, VerticesCPU.Data, VerticesCPU.GetDataBytes());
		::memcpy(static_cast<unsigned char*>(p) + VerticesCPU.GetDataBytes(),
			IndicesCPU.Data, IndicesCPU.GetDataBytes());
		StagingBufferGPU->Unmap(0, nullptr);

		// Transition GPU buffers from COMMON -> COPY_DEST before copying
		{
			const CD3DX12_RESOURCE_BARRIER beginBarriers[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(VertexBufferGPU.Get(),
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST),
				CD3DX12_RESOURCE_BARRIER::Transition(IndexBufferGPU.Get(),
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)
			};
			uploadCommandList->ResourceBarrier(2, beginBarriers);
		}

		// Copy data from upload buffer on CPU into the index/vertex buffer on the GPU
		uploadCommandList->CopyBufferRegion(VertexBufferGPU.Get(), 0,
			StagingBufferGPU.Get(), 0, VerticesCPU.GetDataBytes());
		uploadCommandList->CopyBufferRegion(IndexBufferGPU.Get(), 0,
			StagingBufferGPU.Get(), VerticesCPU.GetDataBytes(), IndicesCPU.GetDataBytes());

		// Barriers, batch them together: COPY_DEST -> usage
		const CD3DX12_RESOURCE_BARRIER barriers[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(VertexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
			CD3DX12_RESOURCE_BARRIER::Transition(IndexBufferGPU.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER)
		};

		uploadCommandList->ResourceBarrier(2, barriers);
	}
}

void RenderMeshResourcePC::EnsureAliveThisFrame(RenderPC* rnd)
{
	rnd->EnsureAliveThisFrame(VertexBufferGPU);
	rnd->EnsureAliveThisFrame(IndexBufferGPU);
	rnd->EnsureAliveThisFrame(StagingBufferGPU);
}









///////////////////////////////////////////////////////////////////////////////
void RenderTextureResourcePC::SetupTexture (RenderPC* rnd, ID3D12GraphicsCommandList * uploadCommandList)
{
	if (TextureGPU.Get())
		return ;	// already uploaded to GPU

// 	auto imageData_ = LoadImageFromMemory(RubyTexture, sizeof(RubyTexture),
// 		1 /* tight row packing */, &Width, &Height);

	static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);
	const auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Width, Height, 1, 1);

	auto& device_ = rnd->m_NativeDevice->device_;

	device_->CreateCommittedResource (&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS (&TextureGPU));

	const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
	const auto uploadBufferSize = GetRequiredIntermediateSize (TextureGPU.Get (), 0, 1);
	const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

	device_->CreateCommittedResource (&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS (&StagingBufferGPU));

	D3D12_SUBRESOURCE_DATA srcData;
	srcData.pData = &(RGBATextureDataCPU->PixelData[0]);
	srcData.RowPitch = Width * RGBAPixel::GetBytesPerPixel();
	srcData.SlicePitch = Width * Height * RGBAPixel::GetBytesPerPixel();

	// Transition texture from COMMON -> COPY_DEST before performing the copy
	{
		const auto transitionToCopy = CD3DX12_RESOURCE_BARRIER::Transition(TextureGPU.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		uploadCommandList->ResourceBarrier(1, &transitionToCopy);
	}
	UpdateSubresources (uploadCommandList, TextureGPU.Get (), StagingBufferGPU.Get (), 0, 0, 1, &srcData);
	const auto transition = CD3DX12_RESOURCE_BARRIER::Transition (TextureGPU.Get (),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	uploadCommandList->ResourceBarrier (1, &transition);

	SRVDescTemplate.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDescTemplate.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDescTemplate.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDescTemplate.Texture2D.MipLevels = 1;
	SRVDescTemplate.Texture2D.MostDetailedMip = 0;
	SRVDescTemplate.Texture2D.ResourceMinLODClamp = 0.0f;
}

void RenderTextureResourcePC::EnsureAliveThisFrame(RenderPC* rnd)
{
	rnd->EnsureAliveThisFrame(TextureGPU);
	rnd->EnsureAliveThisFrame(StagingBufferGPU);
}

void RenderResourcePC::UploadToGPU(RenderPC* rnd, const std::function<void(RenderPC*, ID3D12GraphicsCommandList*)>& callback)
{
	auto& device_ = rnd->m_NativeDevice->device_;
	ComPtr<ID3D12Fence> uploadFence;
	device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));

	ComPtr<ID3D12CommandAllocator> uploadCommandAllocator;
	device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&uploadCommandAllocator));
	ComPtr<ID3D12GraphicsCommandList> uploadCommandList;
	device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, uploadCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&uploadCommandList));

	callback(rnd, uploadCommandList.Get());

	uploadCommandList->Close();
	ID3D12CommandList* commandLists[] = { uploadCommandList.Get() };
	rnd->m_NativeDevice->commandQueue_->ExecuteCommandLists(std::extent<decltype(commandLists)>::value, commandLists);
	rnd->m_NativeDevice->commandQueue_->Signal(uploadFence.Get(), 1);

	auto waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	NativeDevicePC::WaitForFence(uploadFence.Get(), 1, waitEvent);
	uploadCommandAllocator->Reset();
	CloseHandle(waitEvent);
}

void RenderResourcePC::PrepareRender(RenderInterface* rnd)
{
	RenderPC* rndpc = (RenderPC*)rnd;
	UploadToGPU(rndpc, [this](RenderPC* _rnd, ID3D12GraphicsCommandList* _cmdList)
	{
		PrepareRenderImpl(_rnd, _cmdList);
	});
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyD3D12(RenderResource::PrimitiveTypes _type)
{
	switch (_type)
	{
	case RenderResource::Primitive_TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case RenderResource::Primitive_LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	default: assert(false);
	}
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

static D3D_PRIMITIVE_TOPOLOGY GetTopologyD3D(RenderResource::PrimitiveTypes _type)
{
	switch (_type)
	{
	case RenderResource::Primitive_TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case RenderResource::Primitive_LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	default: assert(false);
	}
	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void RenderResourcePC::Render(RenderPC* rndpc, ID3D12GraphicsCommandList* drawCommandList)
{
	RenderMeshResourcePC* meshResPC = (RenderMeshResourcePC*)MeshResource;
	assert(ShaderResource != nullptr);
	auto pso = rndpc->m_NativeDevice->GetPipelineStateObject(this->EnableAlphaBlend, this->EnableDepthTest, this->EnableDepthWrite, this->EnableBackFaceCull, this->IsWireFrameMode, ShaderResource->ShaderType, PrimitiveType);

	// Set our state (shaders, etc.)
	assert(pso.Get() != nullptr);
	drawCommandList->SetPipelineState(pso.Get());

	UploadConstantBuffer(rndpc);

	// Set slot 0 of our root signature to point to our descriptor heap with
	// the texture SRV
	drawCommandList->SetGraphicsRootDescriptorTable(0, rndpc->m_NativeDevice->GetSrvDescriptorGPUHandle(SRVDesciptorTableStartIndex));

	// Set slot 1 of our root signature to the constant buffer view
	drawCommandList->SetGraphicsRootConstantBufferView(1,
		ConstantBufferGPU[rndpc->GetQueueSlot()]->GetGPUVirtualAddress());

	drawCommandList->IASetPrimitiveTopology(GetTopologyD3D(PrimitiveType));
	drawCommandList->IASetVertexBuffers(0, 1, &meshResPC->VertexBufferView);
	drawCommandList->IASetIndexBuffer(&meshResPC->IndexBufferView);
	Draw(drawCommandList);

	FrameNumberToDraw = rndpc->m_NativeDevice->FrameNumberCPU;
}

void RenderResourcePC::Draw(ID3D12GraphicsCommandList* drawCommandList)
{
	drawCommandList->DrawIndexedInstanced(MeshResource->IndicesCPU.NumElements, 1, 0, 0, 0);
}

void RenderResourcePC::Render(RenderInterface* rnd)
{
	Render((RenderPC*)rnd, ((RenderPC*)rnd)->GetDrawCommandList());
}


///////////////////////////////////////////////////////////////////////////////
void RenderResourcePC::PrepareRenderImpl (RenderPC* rnd, ID3D12GraphicsCommandList * uploadCommandList)
{
	RenderMeshResourcePC* meshResPC = (RenderMeshResourcePC*)MeshResource;
	RenderTextureResourcePC* texResPC = (RenderTextureResourcePC*)TextureResource;

	CreatePipelineStateObject (rnd);
	CreateConstantBuffer (rnd);
	assert(meshResPC != nullptr);
	meshResPC->SetupMeshBuffers(rnd, uploadCommandList);
	assert(texResPC != nullptr);
	texResPC->SetupTexture(rnd, uploadCommandList);

	// setup shader resource view
	if (SRVDesciptorTableStartIndex == -1)
	{
		this->SRVDesciptorTableStartIndex = rnd->m_NativeDevice->AllocateSrvDescriptorTable();
	}
	
	int srvIndexOfDescHeap = SRVDesciptorTableStartIndex + NativeDevicePC::SRVTable_Texture_t0;
	if (SRVRawResouce[NativeDevicePC::SRVTable_Texture_t0] != texResPC->TextureGPU.Get())
	{
		rnd->m_NativeDevice->device_->CreateShaderResourceView(texResPC->TextureGPU.Get(), &texResPC->SRVDescTemplate, rnd->m_NativeDevice->GetSrvDescriptorCPUHandle(srvIndexOfDescHeap));
		SRVRawResouce[NativeDevicePC::SRVTable_Texture_t0] = texResPC->TextureGPU.Get();
	}
}

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////


void RenderResourcePC::CreateConstantBuffer (RenderPC* rnd)
{
	auto& device_ = rnd->m_NativeDevice->device_;

	for (int i = 0; i < RenderPC::QUEUE_SLOT_COUNT; ++i) 
	{
		if (ConstantBufferGPU[i].Get() != nullptr)
			continue;

		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
		const auto constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof (ConstantBuffer));

		// These will remain in upload heap because we use them only once per
		// frame.
		device_->CreateCommittedResource (&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS (&ConstantBufferGPU[i]));

		void* p;
		ConstantBufferGPU[i]->Map (0, nullptr, &p);
		::memcpy (p, &ConstBufferCPU, sizeof (ConstantBuffer));
		ConstantBufferGPU[i]->Unmap (0, nullptr);
	}
}

///////////////////////////////////////////////////////////////////////////////
void RenderResourcePC::UploadConstantBuffer(RenderPC* rnd)
{
	void* p;
	ConstantBufferGPU[rnd->GetQueueSlot()]->Map (0, nullptr, &p);
	memcpy(p, &ConstBufferCPU, sizeof(ConstantBuffer));
	ConstantBufferGPU[rnd->GetQueueSlot()]->Unmap (0, nullptr);
}



///////////////////////////////////////////////////////////////////////////////

void RenderResourcePC::CreatePipelineStateObject (RenderPC* rnd)
{
	RenderShaderResourcePC* shaderRes = (RenderShaderResourcePC*)ShaderResource;
	auto& pso = rnd->m_NativeDevice->GetPipelineStateObject(this->EnableAlphaBlend, this->EnableDepthTest, this->EnableDepthWrite, this->EnableBackFaceCull, this->IsWireFrameMode, shaderRes->ShaderType, PrimitiveType);
	if (pso.Get() != nullptr)
		return;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	shaderRes->SetupShader(psoDesc);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC (D3D12_DEFAULT);
	psoDesc.pRootSignature = rnd->m_NativeDevice->rootSignature_.Get();
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.DSVFormat = NativeDevicePC::DepthStencilFormat;

	// D3D uses CW as front
	assert(this->MeshResource != nullptr);
	psoDesc.RasterizerState.FrontCounterClockwise = this->MeshResource->CullMode == RenderMeshResource::CCW_FRONT;
	psoDesc.RasterizerState.CullMode = EnableBackFaceCull ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FillMode = IsWireFrameMode ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

	psoDesc.BlendState = CD3DX12_BLEND_DESC (D3D12_DEFAULT);
	// Simple alpha blending
	psoDesc.BlendState.RenderTarget[0].BlendEnable = this->EnableAlphaBlend;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DepthStencilState.DepthEnable = this->EnableDepthTest;
	psoDesc.DepthStencilState.DepthWriteMask = this->EnableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = rnd->m_NativeDevice->DepthZDir == NativeDevice::FAR_IS_ZERO ? D3D12_COMPARISON_FUNC_GREATER 
        								: (shaderRes->ShaderType == RenderShaderResource::ShaderType_Skybox 
            							? D3D12_COMPARISON_FUNC_LESS_EQUAL 
            							: D3D12_COMPARISON_FUNC_LESS);
	// psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.SampleMask = 0xFFFFFFFF;
	psoDesc.PrimitiveTopologyType = GetTopologyD3D12(PrimitiveType);

	auto& device_ = rnd->m_NativeDevice->device_;
	device_->CreateGraphicsPipelineState (&psoDesc, IID_PPV_ARGS (&pso));
}

void RenderResourcePC::WaitForRenderDone(RenderInterface* rnd)
{
	RenderPC* rndpc = (RenderPC*)rnd;
	auto waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(waitEvent != nullptr);
	NativeDevicePC::WaitForFence(rndpc->m_NativeDevice->FrameNumberGPU.Get(), FrameNumberToDraw, waitEvent);
	CloseHandle(waitEvent);
}

void RenderResourcePC::ExitRender(RenderInterface* rnd, bool waitForRenderDone)
{
	if(waitForRenderDone)
		WaitForRenderDone(rnd);	// TODO: use AddPendingDelete(...)? which will be pending deleted?

	if (SRVDesciptorTableStartIndex >= 0)
	{
		RenderPC* rndpc = (RenderPC*)rnd;
		rndpc->m_NativeDevice->FreeSrvDescriptorTable(SRVDesciptorTableStartIndex);
		SRVDesciptorTableStartIndex = -1;
	}
}










///////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderShaderResourcePC::SetupShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	if (VertexShader.Get())
	{
		VertexShader.Reset();
	}
	if (PixelShader.Get())
	{
		PixelShader.Reset();
	}

	switch (ShaderType)
	{
	case RenderShaderResource::ShaderType_OpaqueBasic:
		SetupOpaqueBasicShader(psoDesc);
		break;
	case RenderShaderResource::ShaderType_Instance3D:
		SetupInstance3DShader(psoDesc);
		break;
	case RenderShaderResource::ShaderType_Instance2D:
		SetupInstance2DShader(psoDesc);
		break;
	case RenderShaderResource::ShaderType_Skybox:
		SetupSkyboxShader(psoDesc);
		break;
	case RenderShaderResource::ShaderType_TransparentBasic:
		SetTransparentBasicShader(psoDesc);
		break;
	default:
		assert(false);
	}
}

void RenderShaderResourcePC::EnsureAliveThisFrame(RenderPC* rnd)
{
	rnd->EnsureAliveThisFrame(VertexShader);
	rnd->EnsureAliveThisFrame(PixelShader);
}

static void OnShaderCompileError(Microsoft::WRL::ComPtr<ID3DBlob>& err)
{
	if (err.Get())
	{
		char* shaderErrorText = (char*)err->GetBufferPointer();
		printf("shader error: %s", shaderErrorText);
		std::cout.flush();
		assert(false);
	}
}

void RenderShaderResourcePC::SetupOpaqueBasicShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	static const D3D12_INPUT_ELEMENT_DESC opqBasic_layout[] =
	{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // vtxcolor
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // uv
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D_SHADER_MACRO opqBasic_macros[] = {
		{ "D3D12_SAMPLE_TEXTURE", "1" },
		{ nullptr, nullptr }
	};

	SetupShaderInternal(psoDesc, Shader_OpqBasic, sizeof(Shader_OpqBasic), opqBasic_layout, std::extent<decltype(opqBasic_layout)>::value, opqBasic_macros);
}

void RenderShaderResourcePC::SetupInstance3DShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	static const D3D12_INPUT_ELEMENT_DESC instance3D_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
// 		{ "SV_INSTANCEID", 0, DXGI_FORMAT_R32_UINT, 1, 32,	// SV_INSTANCEID is not reading from user provided buffer so no need to declare it
// 		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	static const D3D_SHADER_MACRO instance3D_macros[] = {
		{ "D3D12_SAMPLE_TEXTURE", "1" },
		{ nullptr, nullptr }
	};

	SetupShaderInternal(psoDesc, Shader_Instance3D, sizeof(Shader_Instance3D), instance3D_layout, std::extent<decltype(instance3D_layout)>::value, instance3D_macros);
}

void RenderShaderResourcePC::SetupInstance2DShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	static const D3D12_INPUT_ELEMENT_DESC instance2D_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
// 		{ "SV_INSTANCEID", 0, DXGI_FORMAT_R32_UINT, 1, 32,	// SV_INSTANCEID is not reading from user provided buffer so no need to declare it
// 		D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	static const D3D_SHADER_MACRO instance2D_macros[] = {
		{ "D3D12_SAMPLE_TEXTURE", "1" },
		{ nullptr, nullptr }
	};

	SetupShaderInternal(psoDesc, Shader_Instance2D, sizeof(Shader_Instance2D), instance2D_layout, std::extent<decltype(instance2D_layout)>::value, instance2D_macros);
}

void RenderShaderResourcePC::SetupSkyboxShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	static const D3D12_INPUT_ELEMENT_DESC skybox_layout[] =
	{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // vtxcolor
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // uv
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D_SHADER_MACRO sky_macros[] = {
		{ "D3D12_SAMPLE_TEXTURE", "1" },
		{ nullptr, nullptr }
	};

	SetupShaderInternal(psoDesc, Shader_Skybox, sizeof(Shader_Skybox), skybox_layout, std::extent<decltype(skybox_layout)>::value, sky_macros);
}

void RenderShaderResourcePC::SetTransparentBasicShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	static const D3D12_INPUT_ELEMENT_DESC transparent_layout[] =
	{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // vtxcolor
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },   // uv
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static const D3D_SHADER_MACRO transparent_macros[] = {
		{ "D3D12_SAMPLE_TEXTURE", "1" },
		{ nullptr, nullptr }
	};

	SetupShaderInternal(psoDesc, Shader_OpqBasic, sizeof(Shader_OpqBasic), transparent_layout, std::extent<decltype(transparent_layout)>::value, transparent_macros);
}

void RenderShaderResourcePC::SetupShaderInternal(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, 
												const char* shaderSource, int shaderSourceBytes,
												const D3D12_INPUT_ELEMENT_DESC* layout, int numLayout, const D3D_SHADER_MACRO* macro)
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderErr;
	D3DCompile(shaderSource, shaderSourceBytes,
		"", macro, nullptr,
		"VS_main", "vs_5_0", 0, 0, &VertexShader, &shaderErr);

	OnShaderCompileError(shaderErr);

	D3DCompile(shaderSource, shaderSourceBytes,
		"", macro, nullptr,
		"PS_main", "ps_5_0", 0, 0, &PixelShader, &shaderErr);

	OnShaderCompileError(shaderErr);

	psoDesc.VS.BytecodeLength = VertexShader->GetBufferSize();
	psoDesc.VS.pShaderBytecode = VertexShader->GetBufferPointer();
	psoDesc.PS.BytecodeLength = PixelShader->GetBufferSize();
	psoDesc.PS.pShaderBytecode = PixelShader->GetBufferPointer();
	psoDesc.InputLayout.NumElements = numLayout;
	psoDesc.InputLayout.pInputElementDescs = layout;
}

template<typename ELEMENT_TYPE>
void RenderResourceInstancedPC::ReallocateInstanceBufferGPU(RenderPC* rnd, RenderResourceInstancedPC::InstanceBuffer<ELEMENT_TYPE>& _buffer, int srvTableOffset)
{
	if (_buffer.AllocBytesGPU >= _buffer.GetDataBytesInUseCPU())
		return;

	rnd->EnsureAliveThisFrame(_buffer.DataGPU);

	auto& device_ = rnd->m_NativeDevice->device_;

	if (_buffer.AllocBytesStaging < _buffer.GetDataBytesInUseCPU())
	{
		assert(_buffer.DataStagingFrameNumberCPU < rnd->m_NativeDevice->FrameNumberCPU);	// on pending staging buffer copy
		_buffer.DataStaging.Reset();
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const auto stagingBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(_buffer.GetDataBytesInUseCPU());
		device_->CreateCommittedResource(&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&stagingBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_buffer.DataStaging));
		_buffer.AllocBytesStaging = _buffer.GetDataBytesInUseCPU();
	}

	if (_buffer.AllocBytesGPU < _buffer.GetDataBytesInUseCPU())
	{
		static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(_buffer.GetDataBytesInUseCPU());
        device_->CreateCommittedResource(&defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&_buffer.DataGPU));

		_buffer.AllocBytesGPU = _buffer.GetDataBytesInUseCPU();
	}

	if (SRVRawResouce[srvTableOffset] != _buffer.DataGPU.Get())
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;	// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-structuredbuffer
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = _buffer.NumElementsCPU;
		srvDesc.Buffer.StructureByteStride = _buffer.GetStride();
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		rnd->m_NativeDevice->device_->CreateShaderResourceView(_buffer.DataGPU.Get(), &srvDesc, rnd->m_NativeDevice->GetSrvDescriptorCPUHandle(SRVDesciptorTableStartIndex + srvTableOffset));
		SRVRawResouce[srvTableOffset] = _buffer.DataGPU.Get();
	}
}

template<typename ELEMENT_TYPE>
void RenderResourceInstancedPC::UpdateInstanceBufferGPU(RenderPC* rnd, RenderResourceInstancedPC::InstanceBuffer<ELEMENT_TYPE>& _buffer, ID3D12GraphicsCommandList* commandList)
{
	if (_buffer.NumElementsCPU == 0)
		return;

	if (_buffer.DataGPU.Get() == nullptr)
		return;

	assert(_buffer.DataStaging.Get() != nullptr);
	assert(_buffer.DataStagingFrameNumberCPU < rnd->m_NativeDevice->FrameNumberCPU);	// staging buffer can only hold 1 copy before GPU exec it

	const CD3DX12_RESOURCE_BARRIER barriersBeginCopy[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(_buffer.DataGPU.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST)
	};
	commandList->ResourceBarrier(1, barriersBeginCopy);

	// https://docs.microsoft.com/en-us/windows/win32/direct3d12/upload-and-readback-of-texture-data#copying
	// "Such a technique efficiently supports discrete GPUs and their large amounts of CPU-inaccessible memory, without commonly impairing UMA architectures."
	void* p;
	_buffer.DataStaging->Map(0, nullptr, &p);
	memcpy(p, _buffer.DataCPU, _buffer.GetDataBytesInUseCPU());
	_buffer.DataStaging->Unmap(0, nullptr);

	commandList->CopyBufferRegion(_buffer.DataGPU.Get(), 0, _buffer.DataStaging.Get(), 0, _buffer.GetDataBytesInUseCPU());
	_buffer.DataStagingFrameNumberCPU = rnd->m_NativeDevice->FrameNumberCPU;

	// Barriers, batch them together
	const CD3DX12_RESOURCE_BARRIER barriersEndCopy[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(_buffer.DataGPU.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ)
	};

	commandList->ResourceBarrier(1, barriersEndCopy);
}

void RenderResourceInstancedPC::Render(RenderInterface* rnd)
{
	Render((RenderPC*)rnd, ((RenderPC*)rnd)->GetDrawCommandList());
}

void RenderResourceInstancedPC::EnsureAliveThisFrame(RenderPC* rnd)
{
	rnd->EnsureAliveThisFrame(m_InstancedData3D.DataGPU);
	rnd->EnsureAliveThisFrame(m_InstancedData3D.DataStaging);
	rnd->EnsureAliveThisFrame(m_InstancedData2D.DataGPU);
	rnd->EnsureAliveThisFrame(m_InstancedData2D.DataStaging);
}

void RenderResourceInstancedPC::Draw(ID3D12GraphicsCommandList* drawCommandList)
{
	assert(m_InstancedData3D.NumElementsCPU == 0 || m_InstancedData2D.NumElementsCPU == 0);
	int numInstances = m_InstancedData3D.NumElementsCPU > 0 ? m_InstancedData3D.NumElementsCPU : m_InstancedData2D.NumElementsCPU;
	drawCommandList->DrawIndexedInstanced(MeshResource->IndicesCPU.NumElements, numInstances, 0, 0, 0);
}

void RenderResourceInstancedPC::Render(RenderPC* rndpc, ID3D12GraphicsCommandList* drawCommandList)
{
	ReallocateInstanceBufferGPU(rndpc, m_InstancedData3D, NativeDevicePC::SRVTable_InstanceBuffer3D_t4);
	UpdateInstanceBufferGPU(rndpc, m_InstancedData3D, drawCommandList);

	ReallocateInstanceBufferGPU(rndpc, m_InstancedData2D, NativeDevicePC::SRVTable_InstanceBuffer2D_t5);
	UpdateInstanceBufferGPU(rndpc, m_InstancedData2D, drawCommandList);

	RenderResourcePC::Render(rndpc, drawCommandList);
}
#endif