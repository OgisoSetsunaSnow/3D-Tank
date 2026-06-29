#pragma once

#include "Platform.h"
#include "gameframework/InputManager/KeyCodes.h"

#include <array>
#include <cstdint>
#include <glm/glm.hpp>

#if VTS_PLATFORM_WINDOWS
#include <Windows.h>
#elif VTS_PLATFORM_OSX
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#endif

/**
 * @brief 输入管理器 - 处理键盘和鼠标输入
 * 采用单例模式，每帧更新输入状态，支持跨平台（Windows/macOS）
 *
 * 使用 KeyCode 枚举（定义在 KeyCodes.h）进行按键指定
 * 使用 MouseButton 命名空间的常量进行鼠标按键指定
 */
class InputManager
{
public:
    /**
     * @brief 获取输入管理器单例实例
     * @return 输入管理器引用
     */
    static InputManager& GetInstance()
    {
        static InputManager instance;
        return instance;
    }

    /**
     * @brief 每帧更新输入状态（应在主循环开始处调用）
     */
    void update();

    /**
     * @brief 检查键是否当前被按下
     * @param key 键码
     * @return 键当前是否被按下
     */
    bool isKeyPressed(KeyCode key) const;

    /**
     * @brief 检查键是否在本帧刚被按下
     * @param key 键码
     * @return 键是否在本帧刚按下（从未按→按下的转换）
     */
    bool isKeyJustPressed(KeyCode key) const;

    /**
     * @brief 检查鼠标按键是否当前被按下
     * @param button 鼠标按键
     * @return 按键是否当前被按下
     */
    bool isMouseButtonPressed(MouseButton button) const;

    /**
     * @brief 检查鼠标按键是否在本帧刚被按下
     * @param button 鼠标按键
     * @return 按键是否在本帧刚按下
     */
    bool isMouseButtonJustPressed(MouseButton button) const;

    /**
     * @brief 获取鼠标在窗口内的相对坐标
     * @return 鼠标位置（2D向量）
     */
    glm::vec2 getMousePosition() const;

    glm::vec2 getMouseDelta() const;

	void enableMouseCapture();
    void disableMouseCapture();

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

	std::array<bool, 256> m_currentKeys = {};      // 当前帧键盘状态
	std::array<bool, 256> m_previousKeys = {};     // 前一帧键盘状态
	std::array<bool, 8> m_currentMouseButtons = {};  // 当前帧鼠标按键状态（支持扩展鼠标）
	std::array<bool, 8> m_previousMouseButtons = {}; // 前一帧鼠标按键状态
	int m_mouseX = 0;                                // 鼠标X坐标
	int m_mouseY = 0;                                // 鼠标Y坐标
    glm::vec2 m_mouseDelta = {};
    bool m_mouseCaptured = true;
#if VTS_PLATFORM_WINDOWS
    bool m_rawInputInited = false;
#elif VTS_PLATFORM_OSX
    bool m_macCaptureInited = false;
#endif
};
