#pragma once

#include "InputAction.h"
#include "gameframework/InputManager/KeyCodes.h"

#include <array>
#include <map>
#include <vector>

class IInputSystem;

/**
 * @file InputMapper.h
 * @brief 输入映射管理器 - 将硬件输入映射到游戏动作和轴
 *
 * InputMapper 提供了一个抽象层，将具体的按键/按钮映射到游戏动作
 * 这样游戏逻辑不需要知道具体按键，只需要查询动作状态
 *
 * 优势：
 *  玩家可以自定义按键绑定
 *  支持多个按键绑定到同一动作
 *  支持手柄、键盘等多种输入源
 */

/**
 * @brief 轴绑定配置
 *
 * 定义一个轴如何从按键计算值
 * 支持：
 *  正向按键（+1.0值）
 *  负向按键（-1.0值）
 *  死区（小于此值视为0）
 *  灵敏度缩放
 */
struct AxisBinding
{
    KeyCode positiveKey = KeyCode::W;  // 正向按键（产生 +1.0）
    KeyCode negativeKey = KeyCode::S;  // 负向按键（产生 -1.0）
    float deadzone = 0.1f;             // 死区阈值（0.0 - 1.0）
    float sensitivity = 1.0f;          // 灵敏度系数（默认1.0）
    bool smoothing = false;            // 是否启用平滑插值

    AxisBinding() = default;

    AxisBinding(KeyCode positive, KeyCode negative, float dz = 0.1f, float sens = 1.0f)
        : positiveKey(positive)
        , negativeKey(negative)
        , deadzone(dz)
        , sensitivity(sens)
        , smoothing(false)
    {
    }
};

/**
 * @brief 输入映射管理器
 *
 *
 *  管理动作和轴的绑定配置
 *  每帧查询输入系统并计算动作/轴状态
 *  提供游戏逻辑使用的高层接口
 *  支持运行时修改绑定
 */
class InputMapper
{
public:
    InputMapper();
    ~InputMapper();

    /**
     * @brief 初始化输入映射器
     * @param pInputSystem 底层输入系统指针
     * @return 是否初始化成功
     */
    bool initialize(IInputSystem* pInputSystem);

    /**
     * @brief 每帧更新（在游戏循环中调用）
     * @param deltaTime 帧时间（用于平滑和插值）
     */
    void update(float deltaTime);

    // 动作查询接口（游戏逻辑使用）


    /**
     * @brief 检查动作是否当前激活（按下状态）
     * @param action 游戏动作
     * @return 动作是否激活
     */
    bool isActionActive(GameAction action) const;

    /**
     * @brief 检查动作是否刚刚激活（本帧按下）
     * @param action 游戏动作
     * @return 动作是否刚激活
     */
    bool isActionJustActivated(GameAction action) const;

    /**
     * @brief 检查动作是否刚刚释放（本帧释放）
     * @param action 游戏动作
     * @return 动作是否刚释放
     */
    bool isActionJustReleased(GameAction action) const;

    /**
     * @brief 检查动作是否在缓冲窗口内被激活（输入缓冲）
     *
     * 输入缓冲允许玩家提前按下按键，在一定时间窗口内仍然有效
     * 这可以显著改善游戏手感，减少"吃输入"的问题
     *
     * 使用场景：
     *  - 武器冷却中，玩家提前按下射击键
     *  - 角色正在执行动作，玩家提前输入下一个动作
     *  - 快节奏游戏中需要精确时机的操作
     *
     * @param action 游戏动作
     * @param bufferTime 缓冲窗口时间（秒），默认 0.15 秒
     * @return 动作是否在缓冲窗口内激活
     *
     * @example
     * // 射击冷却中也能接受输入
     * if (m_pInputMapper->isActionActivatedWithinBuffer(GameAction::Fire, 0.15f))
     * {
     *     if (m_weapon.canFire())
     *     {
     *         m_weapon.fire();
     *         m_pInputMapper->consumeBufferedAction(GameAction::Fire); // 消费输入
     *     }
     * }
     */
    bool isActionActivatedWithinBuffer(GameAction action, float bufferTime = 0.15f) const;

    /**
     * @brief 消费缓冲的动作输入（清除缓冲）
     *
     * 当动作被成功执行后，调用此方法清除缓冲，防止重复触发
     *
     * @param action 游戏动作
     */
    void consumeBufferedAction(GameAction action);

    //
    // 轴查询接口（游戏逻辑使用）
    //

    /**
     * @brief 获取轴的当前值
     * @param axis 游戏轴
     * @return 轴值（-1.0 到 +1.0，经过死区和灵敏度处理）
     */
    float getAxisValue(GameAxis axis) const;

    /**
     * @brief 获取轴的原始值（未经死区和灵敏度处理）
     * @param axis 游戏轴
     * @return 原始轴值
     */
    float getAxisRawValue(GameAxis axis) const;


    // 绑定配置接口（用于自定义控制）


    /**
     * @brief 为动作绑定按键
     * @param action 游戏动作
     * @param key 按键码
     * @param replace 是否替换现有绑定（false则添加额外绑定）
     */
    void bindAction(GameAction action, KeyCode key, bool replace = false);

    /**
     * @brief 为动作绑定多个按键
     * @param action 游戏动作
     * @param keys 按键列表
     * @param replace 是否替换现有绑定
     */
    void bindAction(GameAction action, const std::vector<KeyCode>& keys, bool replace = true);

    /**
     * @brief 解除动作的所有绑定
     * @param action 游戏动作
     */
    void unbindAction(GameAction action);

    /**
     * @brief 配置轴绑定
     * @param axis 游戏轴
     * @param binding 轴绑定配置
     */
    void bindAxis(GameAxis axis, const AxisBinding& binding);

    /**
     * @brief 解除轴绑定
     * @param axis 游戏轴
     */
    void unbindAxis(GameAxis axis);

    /**
     * @brief 获取动作当前绑定的所有按键
     * @param action 游戏动作
     * @return 按键列表
     */
    std::vector<KeyCode> getActionBindings(GameAction action) const;

    /**
     * @brief 获取轴的绑定配置
     * @param axis 游戏轴
     * @return 轴绑定配置指针（如果未绑定则返回nullptr）
     */
    const AxisBinding* getAxisBinding(GameAxis axis) const;


    // 配置加载/保存（未来扩展）


    /**
     * @brief 加载默认绑定配置
     */
    void loadDefaultBindings();

    /**
     * @brief 从配置文件加载绑定
     * @param configPath 配置文件路径
     * @return 是否加载成功
     */
    bool loadBindingsFromFile(const char* configPath);

    /**
     * @brief 保存当前绑定到配置文件
     * @param configPath 配置文件路径
     * @return 是否保存成功
     */
    bool saveBindingsToFile(const char* configPath);

    /**
     * @brief 重置所有绑定为默认值
     */
    void resetToDefaults();

	void enableMouseCapture() const;

    void disableMouseCapture() const;

private:
    // 底层输入系统引用
    IInputSystem* m_pInputSystem = nullptr;

    // 动作绑定映射表：GameAction -> 按键列表
    std::map<GameAction, std::vector<KeyCode>> m_actionBindings;

    // 轴绑定映射表：GameAxis -> 轴配置
    std::map<GameAxis, AxisBinding> m_axisBindings;

    // 动作状态缓存（当前帧）
    std::array<bool, static_cast<size_t>(GameAction::COUNT)> m_actionStates = {};

    // 动作状态缓存（前一帧）
    std::array<bool, static_cast<size_t>(GameAction::COUNT)> m_previousActionStates = {};

    // 轴值缓存（当前帧）
    std::array<float, static_cast<size_t>(GameAxis::COUNT)> m_axisValues = {};

    // 轴值缓存（平滑处理用）
    std::array<float, static_cast<size_t>(GameAxis::COUNT)> m_smoothedAxisValues = {};

    // 输入缓冲相关（用于改善手感）
    std::array<float, static_cast<size_t>(GameAction::COUNT)> m_actionActivationTime = {};  // 动作激活时间戳
    float m_currentTime = 0.0f;                                                             // 累计运行时间

    // 内部辅助方法

    /**
     * @brief 更新所有动作状态
     */
    void updateActionStates();

    /**
     * @brief 更新所有轴值
     * @param deltaTime 帧时间
     */
    void updateAxisValues(float deltaTime);

    /**
     * @brief 应用死区处理
     * @param value 原始值
     * @param deadzone 死区阈值
     * @return 处理后的值
     */
    float applyDeadzone(float value, float deadzone) const;

    /**
     * @brief 平滑插值
     * @param current 当前值
     * @param target 目标值
     * @param deltaTime 帧时间
     * @param speed 插值速度
     * @return 插值后的值
     */
    float smoothDamp(float current, float target, float deltaTime, float speed = 10.0f) const;
};
