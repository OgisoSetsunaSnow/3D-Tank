#pragma once

#include "gameframework/InputManager/KeyCodes.h"

#include <cstdint>
#include <glm/glm.hpp>

/*
 输入系统接口跨平台输入抽象层

 */
class IInputSystem
{
public:
    virtual ~IInputSystem() = default;

    /**
     *  每帧更新输入状态
     * 必须在mainloop中每帧调用一次
     */
    virtual void Update() = 0;

    /**
     * @brief 检查键盘按键是否当前被按下
     * @param key 按键码
     * @return 按键当前是否按下
     */
    virtual bool IsKeyPressed(KeyCode key) const = 0;

    /**
     * @brief 检查键盘按键是否在本帧刚被按下
     * @param key 按键码
     * @return 按键是否在本帧刚按下
     */
    virtual bool IsKeyJustPressed(KeyCode key) const = 0;

    /**
     * @brief 检查鼠标按键是否当前被按下
     * @param button 鼠标按键
     * @return 按键当前是否被按下
     */
    virtual bool IsMouseButtonPressed(MouseButton button) const = 0;

    /**
     * @brief 检查鼠标按键是否在本帧刚被按下
     * @param button 鼠标按键
     * @return 按键是否在本帧刚按下
     */
    virtual bool IsMouseButtonJustPressed(MouseButton button) const = 0;

    /**
     * @brief 获取鼠标在窗口内的相对坐标
     * @return 鼠标位置（2D向量）
     */
    virtual glm::vec2 GetMousePosition() const = 0;

    static IInputSystem* Create();

    static void Destroy(IInputSystem* pSystem);

	virtual glm::vec2 GetMouseDelta() const = 0;

	virtual void enableMouseCapture() const = 0;

    virtual void disableMouseCapture() const = 0;
};
