#pragma once

#include <Platform.h>

#if VTS_PLATFORM_WINDOWS

#include "render/PC/RenderPC.h"
#include "render/PC/RenderPCNative.h"
#include <vector>
#include <functional>

class RenderMeshResourcePC : public RenderMeshResource
{
	friend class RenderResourcePC;
	friend class RenderPC;

public:
	void SetupMeshBuffers(RenderPC* rnd, ID3D12GraphicsCommandList* uploadCommandList);
	void EnsureAliveThisFrame(RenderPC* rnd);

protected:
	bool RecreateMeshBuffers(RenderPC* rnd);

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	unsigned int VertexBufferGPUBytes = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	unsigned int IndexBufferGPUBytes = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> StagingBufferGPU;
	unsigned int StagingBufferGPUBytes = 0;
};

class RenderTextureResourcePC : public RenderTextureResource
{
	friend class RenderResourcePC;
	friend class RenderPC;
protected:
	void SetupTexture(RenderPC* rnd, ID3D12GraphicsCommandList* uploadCommandList);
	void EnsureAliveThisFrame(RenderPC* rnd);

	Microsoft::WRL::ComPtr<ID3D12Resource>	TextureGPU;
	Microsoft::WRL::ComPtr<ID3D12Resource>	StagingBufferGPU;
	D3D12_SHADER_RESOURCE_VIEW_DESC  SRVDescTemplate = {};
};

class RenderResourcePC : public RenderResource
{
	friend class RenderPC;
public:
	virtual void PrepareRender(RenderInterface* rnd) override;
	virtual void Render(RenderInterface* rnd) override;
	virtual void ExitRender(RenderInterface* rnd, bool waitForRenderDone) override;
protected:
	virtual void Render(RenderPC* rndpc, ID3D12GraphicsCommandList* drawCommandList);
	virtual void Draw(ID3D12GraphicsCommandList* drawCommandList);
	static void UploadToGPU(RenderPC* rnd, const std::function<void(RenderPC*, ID3D12GraphicsCommandList*)>& callback);
	void PrepareRenderImpl(RenderPC* rnd, ID3D12GraphicsCommandList* uploadCommandList);
	virtual void WaitForRenderDone(RenderInterface* rnd) override;

	int SRVDesciptorTableStartIndex = -1;
	void* SRVRawResouce[NativeDevicePC::NumSRVTableEntries] = {};

private:
	void CreateConstantBuffer (RenderPC* rnd);
	void UploadConstantBuffer (RenderPC* rnd);
	void CreatePipelineStateObject (RenderPC* rnd);

	Microsoft::WRL::ComPtr<ID3D12Resource> ConstantBufferGPU[RenderPC::QUEUE_SLOT_COUNT];
	UINT64 FrameNumberToDraw = 0;
};


class RenderShaderResourcePC : public RenderShaderResource
{
public:
	RenderShaderResourcePC(ShaderTypes _type) : RenderShaderResource(_type) {}
	/**
	 * @brief 编译Shader 文件
	 * @param psoDesc 
	 */
	void SetupShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void EnsureAliveThisFrame(RenderPC* rnd);

protected:
	Microsoft::WRL::ComPtr<ID3DBlob> VertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> PixelShader;

private:
	void SetupOpaqueBasicShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void SetupInstance3DShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void SetupInstance2DShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void SetupSkyboxShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void SetTransparentBasicShader(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void SetupShaderInternal(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
		const char* shaderSource, int shaderSourceBytes,
		const D3D12_INPUT_ELEMENT_DESC* layout, int numLayout, const D3D_SHADER_MACRO* macro);
};

struct PerInstanceData3D
{
	float xform[4][4];
	float uv_offset_scale[4];
	float color[4];
};

struct PerInstanceData2D
{
	float xform[4][4];
	float uv_offset_scale[4];
	float color[4];
};

class RenderResourceInstancedPC : public RenderResourcePC
{
	friend class RenderPC;

public:
	virtual void Render(RenderInterface* rnd) override;
	void EnsureAliveThisFrame(RenderPC* rnd);

	template<typename ELEMENT_TYPE>
	class InstanceBuffer final
	{
	public:
		ELEMENT_TYPE* DataCPU = nullptr;
		int NumElementsCPU = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> DataGPU;
		Microsoft::WRL::ComPtr<ID3D12Resource> DataStaging;
		size_t AllocBytesCPU = 0;
		size_t AllocBytesGPU = 0;
		size_t AllocBytesStaging = 0;
		UINT64 DataStagingFrameNumberCPU = 0;

		unsigned int GetDataBytesInUseCPU() { return NumElementsCPU * sizeof(ELEMENT_TYPE); }
		unsigned int GetStride() { return sizeof(ELEMENT_TYPE); }
		void Reset()
		{
			if (DataCPU)
			{
				free(DataCPU);
				DataCPU = nullptr;
				NumElementsCPU = 0;
				AllocBytesCPU = 0;
			}
		}

		void ReallocateCPUBuffer(int numElementsRequired)
		{
			if (numElementsRequired * sizeof(ELEMENT_TYPE) > AllocBytesCPU)
			{
				AllocBytesCPU = numElementsRequired * sizeof(ELEMENT_TYPE);
				DataCPU = (ELEMENT_TYPE*)realloc(DataCPU, AllocBytesCPU);
			}
		}

		void TryReallocateCPUBuffer(int numElemPreAllocate = 16)
		{
			if (GetDataBytesInUseCPU() + GetStride() > AllocBytesCPU)
			{
				ReallocateCPUBuffer((NumElementsCPU + numElemPreAllocate) / numElemPreAllocate * numElemPreAllocate);
			}
		}

		~InstanceBuffer() { Reset(); }
	};

protected:
	InstanceBuffer<PerInstanceData3D> m_InstancedData3D;
	InstanceBuffer<PerInstanceData2D> m_InstancedData2D;

	virtual void Render(RenderPC* rndpc, ID3D12GraphicsCommandList* drawCommandList) override;
	virtual void Draw(ID3D12GraphicsCommandList* drawCommandList) override;

	template<typename ELEMENT_TYPE>
	void ReallocateInstanceBufferGPU(RenderPC* rnd, RenderResourceInstancedPC::InstanceBuffer<ELEMENT_TYPE>& _buffer, int srvTableOffset);
	template<typename ELEMENT_TYPE>
	static void UpdateInstanceBufferGPU(RenderPC* rnd, RenderResourceInstancedPC::InstanceBuffer<ELEMENT_TYPE>& _buffer, ID3D12GraphicsCommandList* commandList);
};

#endif