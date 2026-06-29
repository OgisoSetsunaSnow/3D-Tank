#include "render/PC/RenderPC.h"

#if VTS_PLATFORM_WINDOWS
#include "assets/pc_shader/Shaders.h"
#include "d3dx12.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "render/BMfont.h"
#include "render/PC/ImageIO.h"
#include "render/PC/RenderPCNative.h"
#include "render/PC/RenderResourcePC.h"
#include "render/PC/Window.h"

#include <algorithm>
#include <iostream>

#include <assert.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


using namespace Microsoft::WRL;

namespace
{
struct RenderEnvironment
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain> swapChain;
};

///////////////////////////////////////////////////////////////////////////////
/**
Create everything we need for rendering, this includes a device, a command queue,
and a swap chain.
*/
RenderEnvironment CreateDeviceAndSwapChainHelper(_In_opt_ IDXGIAdapter* adapter,
                                                 D3D_FEATURE_LEVEL minimumFeatureLevel,
                                                 _In_ const DXGI_SWAP_CHAIN_DESC* swapChainDesc)
{
    RenderEnvironment result;

    auto hr = D3D12CreateDevice(adapter, minimumFeatureLevel, IID_PPV_ARGS(&result.device));

    if (FAILED(hr))
    {
        throw std::runtime_error("Device creation failed.");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = result.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&result.queue));

    if (FAILED(hr))
    {
        throw std::runtime_error("Command queue creation failed.");
    }

    ComPtr<IDXGIFactory4> dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

    if (FAILED(hr))
    {
        throw std::runtime_error("DXGI factory creation failed.");
    }

    // Must copy into non-const space
    DXGI_SWAP_CHAIN_DESC swapChainDescCopy = *swapChainDesc;
    hr = dxgiFactory->CreateSwapChain(result.queue.Get(), &swapChainDescCopy, &result.swapChain);

    if (FAILED(hr))
    {
        throw std::runtime_error("Swap chain creation failed.");
    }

    return result;
}
}  // namespace


class FrameResourcePC : public FrameResource
{
    friend class RenderPC;

public:
    void BeginFrame(int backbufferIndex);

protected:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[RenderPC::QUEUE_SLOT_COUNT];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandLists_[RenderPC::QUEUE_SLOT_COUNT];
};

void FrameResourcePC::BeginFrame(int backbufferIndex)
{
    commandAllocators_[backbufferIndex]->Reset();
    auto commandList = commandLists_[backbufferIndex].Get();
    commandList->Reset(commandAllocators_[backbufferIndex].Get(), nullptr);
}

///////////////////////////////////////////////////////////////////////////////
RenderInterface* RenderInterface::CreateRender()
{
    return new RenderPC();
}

RenderPC::RenderPC()
{
    m_NativeDevice = new NativeDevicePC();
    m_FrameResource = new FrameResourcePC();
}

///////////////////////////////////////////////////////////////////////////////
RenderPC::~RenderPC()
{
    if (m_DefaultFontTexture)
    {
        delete m_DefaultFontTexture;
        m_DefaultFontTexture = nullptr;
    }
    if (m_DefaultFont)
    {
        delete m_DefaultFont;
        m_DefaultFont = nullptr;
    }
    delete m_NativeDevice;
    delete m_FrameResource;
}

ID3D12GraphicsCommandList* RenderPC::GetDrawCommandList()
{
    return m_FrameResource->commandLists_[currentBackBuffer_].Get();
}

void RenderPC::EnsureAliveThisFrame(Microsoft::WRL::ComPtr<ID3D12Resource>& res)
{
    if (res.Get())
    {
        m_EnsureAliveResources.push_back(res);
    }
}

void RenderPC::EnsureAliveThisFrame(Microsoft::WRL::ComPtr<ID3DBlob>& res)
{
    if (res.Get())
    {
        m_EnsureAliveBlobs.push_back(res);
    }
}

void NativeDevicePC::WaitForFence(ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent)
{
    if (fence->GetCompletedValue() < completionValue)
    {
        fence->SetEventOnCompletion(completionValue, waitEvent);
        WaitForSingleObject(waitEvent, INFINITE);
    }
}

void NativeDevicePC::WaitBeforeFrameBegin(int backBufferIndex)
{
    WaitForFence(frameFences_[backBufferIndex].Get(),
                 fenceValues_[backBufferIndex],
                 frameFenceEvents_[backBufferIndex]);
}

void NativeDevicePC::BeginFrame(ID3D12GraphicsCommandList* commandList, int backBufferIndex)
{
    ++FrameNumberCPU;

    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(renderTargetHandle,
                                                 renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                                                 backBufferIndex,
                                                 renderTargetViewDescriptorSize_);

    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(depthStencilHandle,
                                                 depthStencilDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                                                 backBufferIndex,
                                                 depthStencilViewDescriptorSize_);

    commandList->OMSetRenderTargets(1, &renderTargetHandle, true, &depthStencilHandle);
    commandList->RSSetViewports(1, &viewport_);
    commandList->RSSetScissorRects(1, &rectScissor_);

    // Transition back buffer
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets_[backBufferIndex].ColorTarget.Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &barrier);

    static const float clearColor[] = {0.042f, 0.042f, 0.042f, 1};

    commandList->ClearRenderTargetView(renderTargetHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(depthStencilHandle,
                                       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       DepthZDir == NativeDevice::FAR_IS_ZERO ? 0.0f : 1.0f,
                                       0,
                                       0,
                                       nullptr);

    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // Set the descriptor heap containing the texture srv
    ID3D12DescriptorHeap* heaps[] = {srvDescriptorHeap_.Get()};
    commandList->SetDescriptorHeaps(1, heaps);
}

void NativeDevicePC::EndFrame(ID3D12GraphicsCommandList* commandList, int backBufferIndex)
{
    // Transition the swap chain back to present
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Transition.pResource = renderTargets_[backBufferIndex].ColorTarget.Get();
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();

    // Execute our commands
    ID3D12CommandList* commandLists[] = {commandList};
    commandQueue_->ExecuteCommandLists(std::extent<decltype(commandLists)>::value, commandLists);
    commandQueue_->Signal(FrameNumberGPU.Get(), FrameNumberCPU);
}

void RenderPC::BeginFrame()
{
    if (m_NativeDevice->window_)
    {
        m_NativeDevice->window_->PumpEvents();
    }
    m_NativeDevice->WaitBeforeFrameBegin(currentBackBuffer_);

    m_EnsureAliveResources.clear();
    m_EnsureAliveBlobs.clear();

    m_FrameResource->BeginFrame(currentBackBuffer_);

    m_NativeDevice->BeginFrame(GetDrawCommandList(), currentBackBuffer_);

    for (int q = 0; q < NumQuadDrawTypes; ++q)
    {
        auto& quadDraws = m_QuadDraws[q][m_NativeDevice->FrameNumberCPU % QUEUE_SLOT_COUNT];
        for (int t = 0; t < quadDraws.size(); ++t)
        {
            quadDraws[t]->ExitRender(this, false);
        }
        quadDraws.clear();
    }
    m_imguiManager.beginFrame();
}

void RenderPC::EndFrame()
{
    FlushEndFrameDraws(GetDrawCommandList());
    m_imguiManager.endFrame(GetDrawCommandList(),
                            m_NativeDevice->srvDescriptorHeap_.Get(),
                            m_NativeDevice->rootSignature_.Get());
    m_NativeDevice->EndFrame(GetDrawCommandList(), currentBackBuffer_);
    m_NativeDevice->Present(currentBackBuffer_);
}

RenderResource* RenderPC::CreateRenderResource()
{
    return new RenderResourcePC();
}

RenderMeshResource* RenderPC::CreateRenderMeshResource()
{
    return new RenderMeshResourcePC();
}

RenderTextureResource* RenderPC::CreateRenderTextureResource()
{
    return new RenderTextureResourcePC();
}

RenderTextureResource* RenderPC::CreateRenderDefaultTextureResource()
{
    assert(DefaultTexture != nullptr);
    return DefaultTexture;
}

RenderShaderResource* RenderPC::CreateRenderShaderResource(RenderShaderResource::ShaderTypes _type)
{
    return new RenderShaderResourcePC(_type);
}

///////////////////////////////////////////////////////////////////////////////
/**
Setup all render targets. This creates the render target descriptor heap and
render target views for all render targets.

This function does not use a default view but instead changes the format to
_SRGB.
*/
void NativeDevicePC::CreateRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = RenderPC::QUEUE_SLOT_COUNT;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&renderTargetDescriptorHeap_));

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc2 = {};
    heapDesc2.NumDescriptors = RenderPC::QUEUE_SLOT_COUNT;
    heapDesc2.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc2.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap(&heapDesc2, IID_PPV_ARGS(&depthStencilDescriptorHeap_));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart()};
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{depthStencilDescriptorHeap_->GetCPUDescriptorHandleForHeapStart()};

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    swapChain_->GetDesc(&swapChainDesc);

    static const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto dsResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DepthStencilFormat,
                                                  swapChainDesc.BufferDesc.Width,
                                                  swapChainDesc.BufferDesc.Height,
                                                  1,
                                                  1);
    dsResDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    for (int i = 0; i < RenderPC::QUEUE_SLOT_COUNT; ++i)
    {
        D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
        viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipSlice = 0;
        viewDesc.Texture2D.PlaneSlice = 0;

        device_->CreateRenderTargetView(renderTargets_[i].ColorTarget.Get(), &viewDesc, rtvHandle);
        rtvHandle.Offset(renderTargetViewDescriptorSize_);

        device_->CreateCommittedResource(&defaultHeapProperties,
                                         D3D12_HEAP_FLAG_NONE,
                                         &dsResDesc,
                                         D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                         nullptr,
                                         IID_PPV_ARGS(&renderTargets_[i].DepthStencilTarget));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Format = DepthStencilFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        device_->CreateDepthStencilView(renderTargets_[i].DepthStencilTarget.Get(), &dsvDesc, dsvHandle);
        dsvHandle.Offset(depthStencilViewDescriptorSize_);
    }
}

///////////////////////////////////////////////////////////////////////////////
void NativeDevicePC::CreateRootSignature()
{
    // We have two root parameters, one is a pointer to a descriptor heap
    // with a SRV, the second is a constant buffer view
    CD3DX12_ROOT_PARAMETER parameters[2];

    // Create a descriptor table with one entry in our descriptor heap
    CD3DX12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, NumSRVTableEntries, 0};
    parameters[0].InitAsDescriptorTable(1, &range);

    // Our constant buffer view
    parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    // We don't use another descriptor heap for the sampler, instead we use a
    // static sampler
    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;

    // Create the root signature
    descRootSignature.Init(2, parameters, 1, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootBlob;
    ComPtr<ID3DBlob> errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    device_->CreateRootSignature(0,
                                 rootBlob->GetBufferPointer(),
                                 rootBlob->GetBufferSize(),
                                 IID_PPV_ARGS(&rootSignature_));
}

void NativeDevicePC::CreateSrvDescriptorHeap()
{
    // We need one descriptor heap to store our texture SRV which cannot go
    // into the root signature. So create a SRV type heap with one entry
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = MAX_SRV_DESCRIPTORS;
    // This heap contains SRV, UAV or CBVs -- in our case one SRV
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.NodeMask = 0;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap_));
}

///////////////////////////////////////////////////////////////////////////////
/**
Present the current frame by swapping the back buffer, then move to the
next back buffer and also signal the fence for the current queue slot entry.
*/
void NativeDevicePC::Present(int& backBufferIndex)
{
    swapChain_->Present(0, 0);

    // Mark the fence for the current frame.
    const auto fenceValue = currentFenceValue_;
    commandQueue_->Signal(frameFences_[backBufferIndex].Get(), fenceValue);
    fenceValues_[backBufferIndex] = fenceValue;
    ++currentFenceValue_;

    // Take the next back buffer from our chain
    backBufferIndex = (backBufferIndex + 1) % RenderPC::QUEUE_SLOT_COUNT;
}

///////////////////////////////////////////////////////////////////////////////
/**
Set up swap chain related resources, that is, the render target view, the
fences, and the descriptor heap for the render target.
*/
void NativeDevicePC::SetupSwapChain()
{
    currentFenceValue_ = 1;

    // Create fences for each frame so we can protect resources and wait for
    // any given frame
    for (int i = 0; i < RenderPC::QUEUE_SLOT_COUNT; ++i)
    {
        frameFenceEvents_[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fenceValues_[i] = 0;
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences_[i]));
    }

    for (int i = 0; i < RenderPC::QUEUE_SLOT_COUNT; ++i)
    {
        swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i].ColorTarget));
    }

    CreateRenderTargets();
}

///////////////////////////////////////////////////////////////////////////////
bool RenderPC::ShouldQuit()
{
    return m_NativeDevice && m_NativeDevice->window_ && m_NativeDevice->window_->IsClosed();
}

int RenderPC::Init()
{
    m_NativeDevice->window_.reset(new Window("3DTankGame", 1280, 720));

    m_NativeDevice->CreateDeviceAndSwapChain();
    CreateAllocatorsAndCommandLists();
    m_NativeDevice->CreateViewportScissor();

    m_NativeDevice->CreateSrvDescriptorHeap();
    m_NativeDevice->CreateRootSignature();

    {
        DefaultTexture = CreateRenderTextureResource();
        DefaultTexture->CreateDefaultTextureData();
        RenderResourcePC::UploadToGPU(this,
                                      [this](RenderPC* _rnd, ID3D12GraphicsCommandList* _cmdList)
                                      {
                                          RenderTextureResourcePC* defaulTexturePC =
                                              (RenderTextureResourcePC*) (this->DefaultTexture);
                                          defaulTexturePC->SetupTexture(_rnd, _cmdList);
                                      });
    }

    CreateBuiltInDrawResources();

    m_imguiManager.init(m_NativeDevice->device_.Get(),
                        m_NativeDevice->commandQueue_.Get(),
                        m_NativeDevice->window_->GetHWND(),
                        m_NativeDevice->window_.get(),
                        QUEUE_SLOT_COUNT);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void RenderPC::Terminate()
{
    // Ensure persistent render resources free any SRV descriptor tables
    // and wait for GPU to finish using them before deleting CPU objects.

    // Exit and free any quad-draw resources still held in the per-frame lists
    for (int t = 0; t < NumQuadDrawTypes; ++t)
    {
        for (int s = 0; s < QUEUE_SLOT_COUNT; ++s)
        {
            auto& drawList = m_QuadDraws[t][s];
            for (auto& up : drawList)
            {
                if (up)
                    up->ExitRender(this, true);
            }
            drawList.clear();
        }
    }

    if (m_DebugDrawLineResource)
    {
        m_DebugDrawLineResource->ExitRender(this, true);
        if (m_DebugDrawLineResource->MeshResource)
        {
            delete m_DebugDrawLineResource->MeshResource;
            m_DebugDrawLineResource->MeshResource = nullptr;
        }
        if (m_DebugDrawLineResource->ShaderResource)
        {
            delete m_DebugDrawLineResource->ShaderResource;
            m_DebugDrawLineResource->ShaderResource = nullptr;
        }
        delete m_DebugDrawLineResource;
        m_DebugDrawLineResource = nullptr;
    }

    if (m_DebugDrawSphereResource)
    {
        m_DebugDrawSphereResource->ExitRender(this, true);
        if (m_DebugDrawSphereResource->MeshResource)
        {
            delete m_DebugDrawSphereResource->MeshResource;
            m_DebugDrawSphereResource->MeshResource = nullptr;
        }
        if (m_DebugDrawSphereResource->ShaderResource)
        {
            delete m_DebugDrawSphereResource->ShaderResource;
            m_DebugDrawSphereResource->ShaderResource = nullptr;
        }
        delete m_DebugDrawSphereResource;
        m_DebugDrawSphereResource = nullptr;
    }

    // Delete quad mesh and shaders
    if (m_QuadMesh)
    {
        delete m_QuadMesh;
        m_QuadMesh = nullptr;
    }
    for (int i = 0; i < NumQuadDrawTypes; ++i)
    {
        if (m_QuadShader[i])
        {
            delete m_QuadShader[i];
            m_QuadShader[i] = nullptr;
        }
    }

    // Delete default texture after resources that reference it were exited
    if (DefaultTexture)
    {
        delete DefaultTexture;
        DefaultTexture = nullptr;
    }

    // Drain the queue, wait for everything to finish
    for (int i = 0; i < GetQueueSlotCount(); ++i)
    {
        NativeDevicePC::WaitForFence(m_NativeDevice->frameFences_[i].Get(),
                                     m_NativeDevice->fenceValues_[i],
                                     m_NativeDevice->frameFenceEvents_[i]);
    }

    for (auto event : m_NativeDevice->frameFenceEvents_)
    {
        CloseHandle(event);
    }

    m_NativeDevice->window_->setMessageHandler(nullptr);

    m_imguiManager.shutdown(m_NativeDevice->window_.get());
}

void RenderPC::CreateBuiltInDrawResources()
{
    m_DebugDrawLineResource = new RenderResourcePC();
    m_DebugDrawLineResource->PrimitiveType = RenderResource::Primitive_LineList;
    m_DebugDrawLineResource->TextureResource = DefaultTexture;
    m_DebugDrawLineResource->MeshResource = CreateRenderMeshResource();
    m_DebugDrawLineResource->ShaderResource = CreateRenderShaderResource(RenderShaderResource::ShaderType_OpaqueBasic);

    m_DebugDrawLineResource->MeshResource->VerticesCPU.NumElements = MAX_NUM_DEBUG_DRAW_LINES * 2;
    m_DebugDrawLineResource->MeshResource->VerticesCPU.AllocBytes =
        sizeof(RenderMeshResource::MeshVertex) * m_DebugDrawLineResource->MeshResource->VerticesCPU.NumElements;
    m_DebugDrawLineResource->MeshResource->VerticesCPU.Data =
        (RenderMeshResource::MeshVertex*) malloc(m_DebugDrawLineResource->MeshResource->VerticesCPU.AllocBytes);

    m_DebugDrawLineResource->MeshResource->IndicesCPU.NumElements = MAX_NUM_DEBUG_DRAW_LINES * 2;
    m_DebugDrawLineResource->MeshResource->IndicesCPU.AllocBytes =
        sizeof(int) * m_DebugDrawLineResource->MeshResource->IndicesCPU.NumElements;
    m_DebugDrawLineResource->MeshResource->IndicesCPU.Data =
        (int*) malloc(m_DebugDrawLineResource->MeshResource->IndicesCPU.AllocBytes);
    m_DebugDrawLineResource->MeshResource->IsDynamicMesh = true;

    for (int k = 0; k < MAX_NUM_DEBUG_DRAW_LINES; ++k)
    {
        m_DebugDrawLineResource->MeshResource->IndicesCPU.Data[k] = k;
    }

    m_DebugDrawSphereResource = new RenderResourceInstancedPC();
    m_DebugDrawSphereResource->TextureResource = DefaultTexture;
    m_DebugDrawSphereResource->MeshResource = CreateRenderMeshResource();
    m_DebugDrawSphereResource->MeshResource->LoadMesh_Sphere(0.5f, 16, 16);
    m_DebugDrawSphereResource->ShaderResource = CreateRenderShaderResource(RenderShaderResource::ShaderType_Instance3D);
    m_DebugDrawSphereResource->m_InstancedData3D.ReallocateCPUBuffer(1024);  // preserve but can grow again
    m_DebugDrawSphereResource->m_InstancedData2D.ReallocateCPUBuffer(1024);
    m_DebugDrawSphereResource->PrimitiveType = RenderResource::Primitive_LineList;

    m_QuadMesh = new RenderMeshResourcePC();
    m_QuadMesh->LoadMesh_Plane();
    m_QuadShader[QuadDraw3D_DepthEnable] = new RenderShaderResourcePC(RenderShaderResource::ShaderType_Instance3D);
    m_QuadShader[QuadDraw3D_DepthDisable] = new RenderShaderResourcePC(RenderShaderResource::ShaderType_Instance3D);
    m_QuadShader[QuadDraw2D] = new RenderShaderResourcePC(RenderShaderResource::ShaderType_Instance2D);
}

void RenderPC::SetViewProjection(float (&viewProj)[4][4])
{
    memcpy(this->ViewProj, viewProj, sizeof(viewProj));
}

void RenderPC::SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture)
{
    m_DefaultFont = fontDesc;
    m_DefaultFontTexture = texture;
    m_DefaultFont->PostLoadFont(texture->Width, texture->Height);
}

void RenderPC::DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB)
{
    DebugDraw::DrawArgs_Sphere s;
    memcpy(s.Pos, posXYZ, sizeof(s.Pos));
    memcpy(s.Scale, scaleXYZ, sizeof(s.Scale));
    memcpy(s.Color, colorRGB, sizeof(s.Color));
    DebugDrawSpheres.push_back(s);
}

void RenderPC::DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB)
{
    DebugDraw::DrawArgs_Line li;
    memcpy(li.StartPos, pos0XYZ, sizeof(li.StartPos));
    memcpy(li.EndPos, pos1XYZ, sizeof(li.EndPos));
    memcpy(li.Color, colorRGB, sizeof(li.Color));
    DebugDrawLines.push_back(li);
}

size_t RenderPC::SetupDebugDrawSphere()
{
    m_DebugDrawSphereResource->m_InstancedData3D.ReallocateCPUBuffer((int) DebugDrawSpheres.size());
    m_DebugDrawSphereResource->m_InstancedData3D.NumElementsCPU = (int) DebugDrawSpheres.size();
    float _uv_offset_scale[4] = {0, 0, 1.0f, 1.0f};

    for (int k = 0; k < DebugDrawSpheres.size(); ++k)
    {
        float* _pos = DebugDrawSpheres[k].Pos;
        float* _scale = DebugDrawSpheres[k].Scale;
        float xform[4][4] = {{_scale[0], 0, 0, 0},
                             {0, _scale[1], 0, 0},
                             {0, 0, _scale[2], 0},
                             {_pos[0], _pos[1], _pos[2], 1}};
        RenderBufferUtil::CopyShaderMatrix(m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].xform, xform);
        memcpy(m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].uv_offset_scale,
               _uv_offset_scale,
               sizeof(float) * 4);
        memcpy(m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].color,
               DebugDrawSpheres[k].Color,
               sizeof(float) * 3);
        m_DebugDrawSphereResource->m_InstancedData3D.DataCPU[k].color[3] = 1;
    }

    size_t num = DebugDrawSpheres.size();
    DebugDrawSpheres.clear();
    return num;
}

size_t RenderPC::SetupDebugDrawLine()
{
    m_DebugDrawLineResource->MeshResource->VerticesCPU.Resize((int) DebugDrawLines.size() * 2);
    m_DebugDrawLineResource->MeshResource->IndicesCPU.Resize((int) DebugDrawLines.size() * 2);

    float _uv[2] = {0, 0};
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

    size_t num = DebugDrawLines.size();
    DebugDrawLines.clear();
    return num;
}

void RenderPC::FlushEndFrameDraws(ID3D12GraphicsCommandList* drawCommandList)
{
    FlushQuadDraws(drawCommandList);

    size_t numSpheres = SetupDebugDrawSphere();
    if (numSpheres > 0)
    {
        m_DebugDrawSphereResource->PrepareRenderImpl(this, drawCommandList);
        RenderBufferUtil::CopyShaderMatrix(m_DebugDrawSphereResource->ConstBufferCPU.WorldViewProj, this->ViewProj);
        m_DebugDrawSphereResource->Render(this, drawCommandList);
    }


    size_t numLines = SetupDebugDrawLine();
    if (numLines > 0)
    {
        m_DebugDrawLineResource->PrepareRenderImpl(this, drawCommandList);
        RenderBufferUtil::CopyShaderMatrix(m_DebugDrawLineResource->ConstBufferCPU.WorldViewProj, this->ViewProj);
        m_DebugDrawLineResource->Render(this, drawCommandList);
    }
}

RenderResourceInstancedPC* RenderPC::GetOrCreateNextRenderResourceInstanced(QuadDrawType drawType,
                                                                            RenderTextureResource* texture)
{
    RenderResourceInstancedPC* res = nullptr;
    auto& drawlist = m_QuadDraws[drawType][m_NativeDevice->FrameNumberCPU % QUEUE_SLOT_COUNT];
    for (int k = 0; k < drawlist.size(); ++k)
    {
        if (drawlist[k]->TextureResource == texture)  // combine batch if texture is the same
        {
            res = drawlist[k].get();
            break;
        }
    }
    if (res == nullptr)
    {
        drawlist.emplace_back(new RenderResourceInstancedPC());
        res = drawlist.back().get();
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
            res->EnableAlphaBlend = true;
            break;
        default:
            res->EnableDepthTest = true;
        }
    }
    return res;
}

void RenderPC::DrawQuad3D(float (&worldTransform)[4][4],
                          float (&uvOffsetScale)[4],
                          float (&color)[4],
                          RenderTextureResource* texture,
                          bool isDepthEnable)
{
    RenderResourceInstancedPC* res =
        GetOrCreateNextRenderResourceInstanced(isDepthEnable ? QuadDraw3D_DepthEnable : QuadDraw3D_DepthDisable,
                                               texture);
    res->m_InstancedData3D.TryReallocateCPUBuffer();

    PerInstanceData3D& instanceData = res->m_InstancedData3D.DataCPU[res->m_InstancedData3D.NumElementsCPU];
    RenderBufferUtil::CopyShaderMatrix(instanceData.xform, worldTransform);
    memcpy(instanceData.uv_offset_scale, uvOffsetScale, sizeof(uvOffsetScale));
    memcpy(instanceData.color, color, sizeof(color));
    res->m_InstancedData3D.NumElementsCPU++;
}


void RenderPC::DrawQuad2D(float (&clipTransform)[4][4],
                          float (&uvOffsetScale)[4],
                          float (&color)[4],
                          RenderTextureResource* texture)
{
    RenderResourceInstancedPC* res = GetOrCreateNextRenderResourceInstanced(QuadDraw2D, texture);
    res->m_InstancedData2D.TryReallocateCPUBuffer();

    PerInstanceData2D& instanceData = res->m_InstancedData2D.DataCPU[res->m_InstancedData2D.NumElementsCPU];
    RenderBufferUtil::CopyShaderMatrix(instanceData.xform,
                                       clipTransform);  // use 4x4 as storage for using shader built-in math functions
    memcpy(instanceData.uv_offset_scale, uvOffsetScale, sizeof(uvOffsetScale));
    memcpy(instanceData.color, color, sizeof(color));
    res->m_InstancedData2D.NumElementsCPU++;
}

void RenderPC::FlushQuadDraws(ID3D12GraphicsCommandList* drawCommandList)
{
    for (int t = 0; t < NumQuadDrawTypes; ++t)
    {
        auto& drawList = m_QuadDraws[t][m_NativeDevice->FrameNumberCPU % QUEUE_SLOT_COUNT];
        for (int k = 0; k < drawList.size(); ++k)
        {
            drawList[k]->PrepareRenderImpl(this, drawCommandList);
            RenderBufferUtil::CopyShaderMatrix(drawList[k]->ConstBufferCPU.WorldViewProj, this->ViewProj);
            drawList[k]->Render(this, drawCommandList);
        }
    }
}


struct DrawTextScratchPad
{
    float CharQuadLocalTransform[4][4];  // the xform from origin of the sentence to character quad
    float CharQuadWorldTransform[4][4];  // the xform from origin of 3D world to character quad
    float UVOffsetScale[4];
};
static DrawTextScratchPad* GetDrawTextScratchPad()
{
    static thread_local DrawTextScratchPad* s_DrawTextScratchpad = new DrawTextScratchPad();
    return s_DrawTextScratchpad;
}

void RenderPC::DrawText3D(const char* text,
                          float (&worldTransform)[4][4],
                          float (&color)[4],
                          float fontScale,
                          bool isDepthEnable)
{
    int length = (int) strlen(text);
    assert(length < 1024);  // null terminated ?

    DrawTextScratchPad* scratchPad = GetDrawTextScratchPad();

    const float quadWidth = 1.0f;
    BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
    for (int k = 0; k < length; ++k)
    {
        m_DefaultFont->BuildInstanceData(ctx, text[k], scratchPad->CharQuadLocalTransform, scratchPad->UVOffsetScale);
        RenderBufferUtil::MulMatrix((float*) &(scratchPad->CharQuadLocalTransform),
                                    (float*) &worldTransform,
                                    (float*) &(scratchPad->CharQuadWorldTransform));
        DrawQuad3D(scratchPad->CharQuadWorldTransform,
                   scratchPad->UVOffsetScale,
                   color,
                   m_DefaultFontTexture,
                   isDepthEnable);
    }
}

void RenderPC::DrawText2D(const char* text, float (&clipTransform)[4][4], float (&color)[4], float fontScale)
{
    int length = (int) strlen(text);
    assert(length < 1024);  // null terminated ?

    DrawTextScratchPad* scratchPad = GetDrawTextScratchPad();

    const float quadWidth = 1.0f;
    BMFont::InstanceDataBuildingContext ctx(fontScale, quadWidth);
    for (int k = 0; k < length; ++k)
    {
        m_DefaultFont->BuildInstanceData(ctx, text[k], scratchPad->CharQuadLocalTransform, scratchPad->UVOffsetScale);
        RenderBufferUtil::MulMatrix((float*) &(scratchPad->CharQuadLocalTransform),
                                    (float*) &clipTransform,
                                    (float*) &(scratchPad->CharQuadWorldTransform));
        DrawQuad2D(scratchPad->CharQuadWorldTransform, scratchPad->UVOffsetScale, color, m_DefaultFontTexture);
    }
}

float RenderPC::GetTextTotalWidth(const char* text, float fontScale)
{
    const float quadWidth = 1.0f;
    return m_DefaultFont->GetTextTotalWidth(text, fontScale, quadWidth);
}

///////////////////////////////////////////////////////////////////////////////
void NativeDevicePC::CreateDeviceAndSwapChain()
{
    // Enable the debug layers when in debug mode
    // If this fails, install the Graphics Tools for Windows. On Windows 10,
    // open settings, Apps, Apps & Features, Optional features, Add Feature,
    // and add the graphics tools
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    debugController->EnableDebugLayer();
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ::ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

    swapChainDesc.BufferCount = RenderPC::QUEUE_SLOT_COUNT;
    // This is _UNORM but we'll use a _SRGB view on this. See
    // SetupRenderTargets() for details, it must match what
    // we specify here
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferDesc.Width = window_->GetWidth();
    swapChainDesc.BufferDesc.Height = window_->GetHeight();
    swapChainDesc.OutputWindow = window_->GetHWND();
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Windowed = true;

    auto renderEnv = CreateDeviceAndSwapChainHelper(nullptr, D3D_FEATURE_LEVEL_11_0, &swapChainDesc);

    device_ = renderEnv.device;
    commandQueue_ = renderEnv.queue;
    swapChain_ = renderEnv.swapChain;

    renderTargetViewDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    depthStencilViewDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    shaderResourceViewDescriptorSize_ =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FrameNumberGPU));

    SetupSwapChain();

#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoQueue;
    device_->QueryInterface(IID_PPV_ARGS(&infoQueue));

    D3D12_MESSAGE_ID defaultFilter[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
    };
    D3D12_INFO_QUEUE_FILTER filter;
    ZeroMemory(&filter, sizeof(D3D12_INFO_QUEUE_FILTER));
    filter.DenyList.NumIDs = sizeof(defaultFilter) / sizeof(defaultFilter[0]);
    filter.DenyList.pIDList = defaultFilter;
    infoQueue->AddStorageFilterEntries(&filter);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void RenderPC::CreateAllocatorsAndCommandLists()
{
    for (int i = 0; i < GetQueueSlotCount(); ++i)
    {
        m_NativeDevice->device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                        IID_PPV_ARGS(&m_FrameResource->commandAllocators_[i]));
        m_NativeDevice->device_->CreateCommandList(0,
                                                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                   m_FrameResource->commandAllocators_[i].Get(),
                                                   nullptr,
                                                   IID_PPV_ARGS(&m_FrameResource->commandLists_[i]));
        m_FrameResource->commandLists_[i]->Close();
    }
}

///////////////////////////////////////////////////////////////////////////////
void NativeDevicePC::CreateViewportScissor()
{
    rectScissor_ = {0, 0, window_->GetWidth(), window_->GetHeight()};

    viewport_ =
        {0.0f, 0.0f, static_cast<float>(window_->GetWidth()), static_cast<float>(window_->GetHeight()), 0.0f, 1.0f};
}

int NativeDevicePC::AllocateSrvDescriptorTable()
{
    if (SrvDescriptorIndexFreeList.size() > 0)
    {
        int newIndex = SrvDescriptorIndexFreeList.back();
        SrvDescriptorIndexFreeList.pop_back();
        return newIndex;
    }
    assert(SrvDescriptorIndexFreeListCap + NumSRVTableEntries <
           MAX_SRV_DESCRIPTORS);  // increase MAX_SRV_DESCRIPTORS if you need
    int currentIndex = SrvDescriptorIndexFreeListCap;
    SrvDescriptorIndexFreeListCap += NumSRVTableEntries;
    return currentIndex;
}

void NativeDevicePC::FreeSrvDescriptorTable(int index)
{
    assert(index >= 0);
    assert(std::find(SrvDescriptorIndexFreeList.begin(), SrvDescriptorIndexFreeList.end(), index) ==
           SrvDescriptorIndexFreeList.end());
    assert(index < SrvDescriptorIndexFreeListCap);
#if 0
	for (int k = 0; k < NumSRVTableEntries; ++k)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = 0;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device_->CreateShaderResourceView(nullptr, &srvDesc, GetSrvDescriptorCPUHandle(index + k));	// https://docs.microsoft.com/en-us/windows/win32/direct3d12/descriptors-overview#null-descriptors
	}
#endif
    SrvDescriptorIndexFreeList.push_back(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE NativeDevicePC::GetSrvDescriptorCPUHandle(int index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE _handle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted(_handle,
                                                 srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
                                                 index,
                                                 shaderResourceViewDescriptorSize_);
    return _handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE NativeDevicePC::GetSrvDescriptorGPUHandle(int index)
{
    D3D12_GPU_DESCRIPTOR_HANDLE _handle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE::InitOffsetted(_handle,
                                                 srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart(),
                                                 index,
                                                 shaderResourceViewDescriptorSize_);
    return _handle;
}

union PipelineStateKey
{
    struct PipelineStateKeyDef
    {
        unsigned int ShaderType : 8;
        static_assert(RenderShaderResource::ShaderTypes::NumShaderTypes < 256,
                      "Not enough PipelineStateKeyDef::ShaderType bits");

        unsigned int PrimitiveType : 4;
        static_assert(RenderResource::PrimitiveTypes::NumPrimtiveTypes < 4,
                      "Not enough PipelineStateKeyDef::PrimitiveType bits");

        unsigned int BlendEnable : 1;
        unsigned int DepthTestEnable : 1;
        unsigned int DepthWriteEnable : 1;
        unsigned int BackFaceCullEnable : 1;
        unsigned int IsWireFrameMode : 1;
    } DefFields;
    int Int32Value;
};

Microsoft::WRL::ComPtr<ID3D12PipelineState>&
NativeDevicePC::GetPipelineStateObject(bool isBlendEnable,
                                       bool isDepthTestEnable,
                                       bool isDepthWirteEnable,
                                       bool isBackFaceCullEnable,
                                       bool isWireFrameModel,
                                       RenderShaderResource::ShaderTypes shaderType,
                                       RenderResource::PrimitiveTypes primitive)
{
    PipelineStateKey key;
    key.Int32Value = 0;  // clear unused bit fields
    key.DefFields.BlendEnable = isBlendEnable;
    key.DefFields.DepthTestEnable = isDepthTestEnable;
    key.DefFields.DepthWriteEnable = isDepthWirteEnable;
    key.DefFields.BackFaceCullEnable = isBackFaceCullEnable;
    key.DefFields.PrimitiveType = primitive;
    key.DefFields.ShaderType = shaderType;
    key.DefFields.IsWireFrameMode = isWireFrameModel;
    auto result = psoPool.try_emplace(key.Int32Value, Microsoft::WRL::ComPtr<ID3D12PipelineState>());
    return result.first->second;
}
#endif