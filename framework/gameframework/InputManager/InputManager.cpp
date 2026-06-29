#include "gameframework/InputManager/InputManager.h"
#include "Common.h"

#if VTS_PLATFORM_WINDOWS
#include <Windows.h>
#elif VTS_PLATFORM_OSX
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#endif

void InputManager::update()
{
	// 保存前一帧状态
    m_previousKeys = m_currentKeys;
    m_previousMouseButtons = m_currentMouseButtons;
    m_mouseDelta = glm::vec2(0.f);

#if VTS_PLATFORM_WINDOWS
	// Windows平台：使用Windows API获取键盘和鼠标状态
	for (int i = 0; i < 256; ++i)
	{
		m_currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
	}

	// 鼠标按键检测
	m_currentMouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;  // 左键
	m_currentMouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;  // 右键
	m_currentMouseButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;  // 中键

	// 鼠标位置检测：全局坐标→客户端坐标
    HWND hwnd = GetForegroundWindow();
    if (hwnd)
    {
        // 第一帧有窗口时，注册 Raw Input + 隐藏光标
        if (!m_rawInputInited)
        {
            RAWINPUTDEVICE rid;
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x02;
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = hwnd;
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
            while (ShowCursor(FALSE) >= 0);
            m_rawInputInited = true;
        }

        if (m_mouseCaptured)
        {
            // Raw Input 读 delta
            UINT bufferSize = 0;
            GetRawInputBuffer(nullptr, &bufferSize, sizeof(RAWINPUTHEADER));
            if (bufferSize > 0)
            {
                bufferSize *= 8;
                std::vector<BYTE> buffer(bufferSize);
                UINT count =
                    GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer.data()), &bufferSize, sizeof(RAWINPUTHEADER));
                if (count > 0 && count != (UINT) -1)
                {
                    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
                    for (UINT i = 0; i < count; ++i)
                    {
                        if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
                        {
                            m_mouseDelta.x += static_cast<float>(raw->data.mouse.lLastX);
                            m_mouseDelta.y += static_cast<float>(raw->data.mouse.lLastY);
                        }
                        raw =
                            reinterpret_cast<RAWINPUT*>(reinterpret_cast<BYTE*>(raw) + ((raw->header.dwSize + 7) & ~7));
                    }
                }
            }

            // 锁回中心
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            POINT center;
            center.x = (clientRect.right - clientRect.left) / 2;
            center.y = (clientRect.bottom - clientRect.top) / 2;
            m_mouseX = center.x;
            m_mouseY = center.y;
            ClientToScreen(hwnd, &center);
            SetCursorPos(center.x, center.y);
        }
        else
        {
            // 菜单模式：正常读位置
            POINT mousePos;
            GetCursorPos(&mousePos);
            ScreenToClient(hwnd, &mousePos);
            m_mouseX = mousePos.x;
            m_mouseY = mousePos.y;
        }
    }


#elif VTS_PLATFORM_OSX
    // 1. 键盘
    m_currentKeys.fill(false);
    for (KeyCode key : kAllKeys)
    {
        CGKeyCode cgKey = toCGKeyCode(key);
        if (cgKey == static_cast<CGKeyCode>(0xFF))
            continue;
        m_currentKeys[static_cast<uint8_t>(key)] = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, cgKey);
    }

    // 2. 鼠标按键
    m_currentMouseButtons[0] = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft);
    m_currentMouseButtons[1] = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight);
    m_currentMouseButtons[2] = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter);
    for (int i = 3; i < 8; ++i)
        m_currentMouseButtons[i] = false;

    // 3. 鼠标坐标
    NSWindow* window = [[NSApplication sharedApplication] mainWindow];

    if (m_mouseCaptured)
    {
        // 第一帧：隐藏光标，解除系统鼠标关联
        if (!m_macCaptureInited && window)
        {
            [NSCursor hide];
            CGAssociateMouseAndMouseCursorPosition(false);
            m_macCaptureInited = true;
        }

        // 用 CGGetLastMouseDelta 获取原始硬件 delta（不受 warp 影响）
        int32_t dx = 0, dy = 0;
        CGGetLastMouseDelta(&dx, &dy);
        m_mouseDelta.x = static_cast<float>(dx);
        m_mouseDelta.y = static_cast<float>(dy);  // Y 正方向向下，和 Windows Raw Input 一致

        // 锁回窗口中心（防止光标跑出屏幕）
        if (window)
        {
            NSRect frame = [window frame];
            CGFloat cx = frame.origin.x + frame.size.width * 0.5f;
            CGFloat cy = frame.origin.y + frame.size.height * 0.5f;
            // macOS 屏幕原点在左下，CGWarpMouseCursorPosition 也是左下原点，直接用
            CGWarpMouseCursorPosition(CGPointMake(cx, cy));
        }

        // captured 模式下位置固定在中心，不需要更新 m_mouseX/Y
    }
    else
    {
        // 菜单模式：恢复光标关联，正常读位置
        if (m_macCaptureInited)
        {
            CGAssociateMouseAndMouseCursorPosition(true);
            [NSCursor unhide];
            m_macCaptureInited = false;
        }

        // 翻转 Y 轴，统一左上角为原点
        NSPoint loc = [NSEvent mouseLocation];
        CGFloat screenH = [[NSScreen mainScreen] frame].size.height;
        m_mouseX = static_cast<int>(loc.x);
        m_mouseY = static_cast<int>(screenH - loc.y);

        // 菜单模式不输出 delta，防止相机闪现
        m_mouseDelta = glm::vec2(0.f);
    }
#endif
}

bool InputManager::isKeyPressed(KeyCode key) const
{
	return m_currentKeys[static_cast<uint8_t>(key)];
}

bool InputManager::isKeyJustPressed(KeyCode key) const
{
	uint8_t keyIndex = static_cast<uint8_t>(key);
	return m_currentKeys[keyIndex] && !m_previousKeys[keyIndex];
}

bool InputManager::isMouseButtonPressed(MouseButton button) const
{
	int buttonIndex = static_cast<int>(button);
	if (buttonIndex < 0 || buttonIndex >= 8)
		return false;
	return m_currentMouseButtons[buttonIndex];
}

bool InputManager::isMouseButtonJustPressed(MouseButton button) const
{
	int buttonIndex = static_cast<int>(button);
	if (buttonIndex < 0 || buttonIndex >= 8)
		return false;
	return m_currentMouseButtons[buttonIndex] && !m_previousMouseButtons[buttonIndex];
}

glm::vec2 InputManager::getMousePosition() const
{
	return glm::vec2(static_cast<float>(m_mouseX), static_cast<float>(m_mouseY));
}

glm::vec2 InputManager::getMouseDelta() const
{
    return m_mouseDelta;
}

void InputManager::enableMouseCapture()
{
    m_mouseCaptured = true;
#if VTS_PLATFORM_WINDOWS
    while (ShowCursor(FALSE) >= 0);
#elif VTS_PLATFORM_OSX
    m_macCaptureInited = false;
#endif
}

void InputManager::disableMouseCapture()
{
    m_mouseCaptured = false;
#if VTS_PLATFORM_WINDOWS
    ShowCursor(TRUE);
#elif VTS_PLATFORM_OSX
    if (m_macCaptureInited)
    {
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor unhide];
        m_macCaptureInited = false;
    }
#endif
}