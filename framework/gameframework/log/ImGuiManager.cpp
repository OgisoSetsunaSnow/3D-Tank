#include "gameframework/log/ImGuiManager.h"

#include <imgui.h>
#if VTS_PLATFORM_WINDOWS
#include "render/PC/Window.h"

#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ImGuiManager::init(ID3D12Device* device,
                        ID3D12CommandQueue* commandQueue,
                        HWND hwnd,
                        IWindow* window,
                        int queueSlotCount)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // 创建 ImGui 专属 SRV 堆
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 64;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap));

    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device;
    initInfo.CommandQueue = commandQueue;
    initInfo.NumFramesInFlight = queueSlotCount;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.SrvDescriptorHeap = m_srvHeap.Get();

    initInfo.SrvDescriptorAllocFn =
        [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
    {
        static UINT s_index = 0;
        UINT size = info->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        out_cpu->ptr = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + s_index * size;
        out_gpu->ptr = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + s_index * size;
        s_index++;
    };

    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {};

    ImGui_ImplDX12_Init(&initInfo);

    // 注册消息处理器
    window->setMessageHandler(
        [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> bool
        {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse)
            {
                switch (msg)
                {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEMOVE:
                case WM_MOUSEWHEEL:
                    return true;
                }
            }
            if (io.WantCaptureKeyboard)
            {
                switch (msg)
                {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                    return true;
                }
            }
            return false;
        });
}

void ImGuiManager::beginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::endFrame(ID3D12GraphicsCommandList* cmdList,
                            ID3D12DescriptorHeap* frameSrvHeap,
                            ID3D12RootSignature* frameRootSignature)
{
    m_logView.draw();
    m_hudView.draw();
    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = {m_srvHeap.Get()};
    cmdList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

    cmdList->SetDescriptorHeaps(1, &frameSrvHeap);
    cmdList->SetGraphicsRootSignature(frameRootSignature);
}

void ImGuiManager::shutdown(IWindow* window)
{
    window->setMessageHandler(nullptr);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
#elif VTS_PLATFORM_OSX
#include "imgui_impl_metal.h"
#include "imgui_impl_osx.h"
#include "render/Mac/WindowMac.h"

void ImGuiManager::init(id<MTLDevice> device, WindowMac* window)
{
    m_window = window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplMetal_Init(device);
    NSView* view = window->window.contentView;
    ImGui_ImplOSX_Init(view);
}
void ImGuiManager::beginFrame(MTLRenderPassDescriptor* renderPassDescriptor)
{
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    if (m_window)
    {
        NSView* view = m_window->window.contentView;
        ImGui_ImplOSX_NewFrame(view);
    }
    else
    {
        return;
    }
    ImGui::NewFrame();
}

void ImGuiManager::endFrame(id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> encoder)
{
    m_logView.draw();
    m_hudView.draw();
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
}

void ImGuiManager::shutdown()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext();
}
#endif
