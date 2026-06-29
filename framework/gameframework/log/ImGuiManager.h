#pragma once

#include "game/UI/PlayerHUDView.h"
#include "gameframework/log/LogSystemImGuiView.h"

#include <Platform.h>
#if VTS_PLATFORM_WINDOWS
#include "render/PC/Window.h"

#include <d3d12.h>
#include <wrl.h>

#elif VTS_PLATFORM_OSX
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
struct WindowMac;
#endif

struct IWindow;

class ImGuiManager
{
public:
#if VTS_PLATFORM_WINDOWS
    void init(ID3D12Device* device, ID3D12CommandQueue* commandQueue, HWND hwnd, IWindow* window, int queueSlotCount);

    void beginFrame();

    void endFrame(ID3D12GraphicsCommandList* cmdList,
                  ID3D12DescriptorHeap* frameSrvHeap,
                  ID3D12RootSignature* frameRootSignature);

    void shutdown(IWindow* window);

#elif VTS_PLATFORM_OSX
    void init(id<MTLDevice> device, WindowMac* window);

    void beginFrame(MTLRenderPassDescriptor* renderPassDescriptor);

    void endFrame(id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> encoder);

    void shutdown();
#endif

private:
    LogSystemImGuiView m_logView;
    Game::PlayerHUDView m_hudView;
#if VTS_PLATFORM_WINDOWS
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
#elif VTS_PLATFORM_OSX
    WindowMac* m_window = nullptr;
#endif
};
