
#pragma once

#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <gnmx.h>
#include "assets/ps4_shader/instance_buffer.h"
#include "render/RenderInterface.h"

enum VertexElements
{
	kVertexPosition = 0,
	kVertexColor,
	kVertexUv,
	kVertexElemCount
};

class NativeDevicePS4;

class RenderMeshResourcePS4 : public RenderMeshResource
{
	friend class RenderPS4;
	friend class RenderResourcePS4;
public:
	bool SetupVB(NativeDevicePS4* dvRes);
	void Release(RenderInterface* rnd);
protected:
	sce::Gnm::Buffer m_VertexBuffersGPU[kVertexElemCount];
	int* m_IndexBufferGPUAddress;
	RenderMeshResource::MeshVertex* m_VertexBufferGPUAddress = nullptr;
	unsigned int m_VertexBufferGPUBytes = 0;
	unsigned int m_IndexBufferGPUBytes = 0;
};

class RenderTextureResourcePS4 : public RenderTextureResource
{
	friend class RenderPS4;
	friend class RenderResourcePS4;

public:
	bool SetupTexture(NativeDevicePS4* dvRes);
	void Release(RenderInterface* rnd);
protected:
	sce::Gnm::Texture TextureGPU;
	void* DebugTextureCPUAddress = nullptr;
	int DebugTextureWidth = 0;
	int DebugTextureHeight = 0;
};


class RenderResourcePS4 : public RenderResource
{
	friend class RenderPS4;

public:
	virtual void PrepareRender(RenderInterface* rnd) override;
	virtual void Render(RenderInterface* rnd) override;
	virtual void ExitRender(RenderInterface* rnd, bool waitForRenderDone) override;
protected:
	virtual void SetupNumInstances(RenderInterface* rnd);
	virtual void WaitForRenderDone(RenderInterface* rnd) override;
	
	uint64_t m_FrameNumberToRender = 0;
};


class RenderResourceInstancedPS4 : RenderResourcePS4
{
	friend class RenderPS4;

public:
	virtual void Render(RenderInterface* rnd) override;
	virtual void ExitRender(RenderInterface* rnd, bool waitForRenderDone) override;

	template<typename ELEMENT_TYPE>
	class InstanceBuffer final
	{
	public:
		ELEMENT_TYPE* DataCPU = nullptr;
		size_t AllocBytesCPU = 0;
		unsigned int AllocBytesGPU = 0;
		int NumElementsCPU = 0;
		unsigned int GetDataBytesInUse() { return NumElementsCPU * sizeof(ELEMENT_TYPE); }
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
			if (GetDataBytesInUse() + GetStride() > AllocBytesCPU)
			{
				ReallocateCPUBuffer((NumElementsCPU + numElemPreAllocate) / numElemPreAllocate * numElemPreAllocate);
			}
		}

		ELEMENT_TYPE* DataGPUAddress = nullptr;
		sce::Gnm::Buffer DataGPU;	// allocated from command buffer, no need to release
		~InstanceBuffer() { Reset(); }
	};
protected:
	virtual void SetupNumInstances(RenderInterface* rnd) override;

	InstanceBuffer<PerInstanceData3D> m_InstancedData3D;
	InstanceBuffer<PerInstanceData2D> m_InstancedData2D;

	template<typename ELEMENT_TYPE>
	static void ReallocateInstanceBufferGPU(RenderPS4* rnd, RenderResourceInstancedPS4::InstanceBuffer<ELEMENT_TYPE>& _buffer);
	template<typename ELEMENT_TYPE>
	static void UpdateInstanceBufferGPU(RenderResourceInstancedPS4::InstanceBuffer<ELEMENT_TYPE>& _buffer);
};


class RenderShaderResourcePS4 : public RenderShaderResource
{
	friend class RenderResourcePS4;

public:
	RenderShaderResourcePS4(ShaderTypes _type) : RenderShaderResource(_type) {}
	void ReleaseGPUResource(RenderInterface* rnd);
	void SetupShader(NativeDevicePS4* dvRes);

protected:
	sce::Gnmx::VsShader* m_VertexShader = nullptr;
	sce::Gnmx::PsShader* m_PixelShader = nullptr;
	void* m_FetchShaderMemory;
	uint32_t m_ShaderModifier;

	sce::Gnmx::InputOffsetsCache m_VSInputOffsetsCache;
	sce::Gnmx::InputOffsetsCache m_PSInputOffsetsCache;
private:
	void SetupOpaqueBasicShader(NativeDevicePS4* dvRes);
	void SetupInstance3DShader(NativeDevicePS4* dvRes);
	void SetupInstance2DShader(NativeDevicePS4* dvRes);

	void SetupShaderInternal(const char* vs_path, const char* ps_path, NativeDevicePS4* dvRes, sce::Gnm::FetchShaderInstancingMode* instancingModePerAttr, int numInstanceAttr);
};

#endif