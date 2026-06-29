#pragma once

#include <Platform.h>

#if VTS_PLATFORM_WINDOWS
#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include <memory>
#include <map>
#include "render/RenderInterface.h"

class NativeDevicePC : public NativeDevice
{
	friend class RenderPC;
	friend class RenderResourcePC;
	friend class RenderResourceInstancedPC;
	friend class RenderMeshResourcePC;
	friend class RenderTextureResourcePC;

public:
	void WaitBeforeFrameBegin(int backBufferIndex);
	void BeginFrame(ID3D12GraphicsCommandList* commandList, int backBufferIndex);
	void EndFrame(ID3D12GraphicsCommandList* commandList, int backBufferIndex);

protected:
	void CreateRenderTargets();
	void Present(int& backBufferIndex);
	void SetupSwapChain();
	void CreateDeviceAndSwapChain();
	void CreateViewportScissor();
	void CreateRootSignature();
	void CreateSrvDescriptorHeap();
	int AllocateSrvDescriptorTable();
	void FreeSrvDescriptorTable(int index);
	Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetPipelineStateObject(bool isBlendEnable, bool isDepthTestEnable, bool isDepthWriteEnable, bool isBackFaceCullEnable, bool isWireFrameMode, RenderShaderResource::ShaderTypes shaderType, RenderResource::PrimitiveTypes primitive);
	static void WaitForFence(ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvDescriptorCPUHandle(int index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvDescriptorGPUHandle(int index);

	D3D12_VIEWPORT viewport_;
	D3D12_RECT rectScissor_;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	static const DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	struct RenderTargets
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> ColorTarget;
		Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencilTarget;
	};
	RenderTargets renderTargets_[RenderPC::QUEUE_SLOT_COUNT];
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12Fence> FrameNumberGPU;
	UINT64 FrameNumberCPU = 1;

	HANDLE frameFenceEvents_[RenderPC::QUEUE_SLOT_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Fence> frameFences_[RenderPC::QUEUE_SLOT_COUNT];
	UINT64 currentFenceValue_;
	UINT64 fenceValues_[RenderPC::QUEUE_SLOT_COUNT];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> depthStencilDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	std::map<int, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoPool;

	std::unique_ptr<Window> window_;

	std::int32_t renderTargetViewDescriptorSize_;
	std::int32_t depthStencilViewDescriptorSize_;
	std::int32_t shaderResourceViewDescriptorSize_;

	enum SRVDescriptorTableDefines
	{
		SRVTable_Texture_t0,
		SRVTable_Texture_t1,
		SRVTable_Texture_t2,
		SRVTable_Texture_t3,
		SRVTable_InstanceBuffer3D_t4,
		SRVTable_InstanceBuffer2D_t5,

		NumSRVTableEntries,
	};

	static const int MAX_SRV_DESCRIPTORS = 1024 * 16;
	std::vector<int> SrvDescriptorIndexFreeList;
	int SrvDescriptorIndexFreeListCap = 0;
};
#endif