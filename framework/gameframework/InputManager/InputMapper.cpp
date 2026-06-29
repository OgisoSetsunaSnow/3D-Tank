#include "InputMapper.h"
#include "gameframework/InputManager/IInputSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdio>


// 构造/析构


InputMapper::InputMapper()
{
	// 初始化所有状态为默认值
	m_actionStates.fill(false);
	m_previousActionStates.fill(false);
	m_axisValues.fill(0.0f);
	m_smoothedAxisValues.fill(0.0f);
	
	// 初始化输入缓冲相关数据
	m_actionActivationTime.fill(-999.0f);  // 设置为很久以前，表示未激活
	m_currentTime = 0.0f;
}

InputMapper::~InputMapper()
{
	m_pInputSystem = nullptr;
}


//初始化和更新


bool InputMapper::initialize(IInputSystem* pInputSystem)
{
	if (!pInputSystem)
	{
		printf("InputMapper::initialize - Invalid input system\n");
		return false;
	}

	m_pInputSystem = pInputSystem;

	// 加载默认绑定
	loadDefaultBindings();

	printf("InputMapper::initialize - Success\n");
	return true;
}

void InputMapper::update(float deltaTime)
{
	if (!m_pInputSystem)
	{
		return;
	}

	// 累计时间（用于输入缓冲）
	m_currentTime += deltaTime;

	// 保存前一帧状态
	m_previousActionStates = m_actionStates;

	// 更新动作和轴状态
	updateActionStates();
	updateAxisValues(deltaTime);

	// 鼠标 delta 直接写入相机轴
    glm::vec2 mouseDelta = m_pInputSystem->GetMouseDelta();
    m_axisValues[static_cast<size_t>(GameAxis::CameraHorizontal)] = mouseDelta.x;
    m_axisValues[static_cast<size_t>(GameAxis::CameraVertical)] = mouseDelta.y;

	// 记录动作激活时间（用于输入缓冲）
	for (size_t i = 0; i < static_cast<size_t>(GameAction::COUNT); ++i)
	{
		if (isActionJustActivated(static_cast<GameAction>(i)))
		{
			m_actionActivationTime[i] = m_currentTime;
		}
	}
}


// 动作查询接口


bool InputMapper::isActionActive(GameAction action) const
{
	size_t index = static_cast<size_t>(action);
	if (index >= m_actionStates.size())
	{
		return false;
	}
	return m_actionStates[index];
}

bool InputMapper::isActionJustActivated(GameAction action) const
{
	size_t index = static_cast<size_t>(action);
	if (index >= m_actionStates.size())
	{
		return false;
	}
	// 前一帧未按下，当前帧按下
	return m_actionStates[index] && !m_previousActionStates[index];
}

bool InputMapper::isActionJustReleased(GameAction action) const
{
	size_t index = static_cast<size_t>(action);
	if (index >= m_actionStates.size())
	{
		return false;
	}
	// 前一帧按下，当前帧未按下
	return !m_actionStates[index] && m_previousActionStates[index];
}

bool InputMapper::isActionActivatedWithinBuffer(GameAction action, float bufferTime) const
{
	size_t index = static_cast<size_t>(action);
	if (index >= m_actionActivationTime.size())
	{
		return false;
	}

	// 计算距离激活的时间
	float timeSinceActivation = m_currentTime - m_actionActivationTime[index];
	
	// 如果在缓冲窗口内，则返回 true
	return timeSinceActivation >= 0.0f && timeSinceActivation <= bufferTime;
}

void InputMapper::consumeBufferedAction(GameAction action)
{
	size_t index = static_cast<size_t>(action);
	if (index >= m_actionActivationTime.size())
	{
		return;
	}

	// 清除缓冲：将激活时间设置为很久以前
	m_actionActivationTime[index] = -999.0f;
}


// 轴查询接口

float InputMapper::getAxisValue(GameAxis axis) const
{
	size_t index = static_cast<size_t>(axis);
	if (index >= m_axisValues.size())
	{
		return 0.0f;
	}
	return m_axisValues[index];
}

float InputMapper::getAxisRawValue(GameAxis axis) const
{
	// 对于非平滑轴，直接返回当前值
	// 对于平滑轴，返回未平滑的目标值
	auto it = m_axisBindings.find(axis);
	if (it == m_axisBindings.end())
	{
		return 0.0f;
	}

	const AxisBinding& binding = it->second;
	float value = 0.0f;

	// 检查正向和负向按键
	if (m_pInputSystem->IsKeyPressed(binding.positiveKey))
	{
		value += 1.0f;
	}
	if (m_pInputSystem->IsKeyPressed(binding.negativeKey))
	{
		value -= 1.0f;
	}

	return value;
}


// 绑定配置接口


void InputMapper::bindAction(GameAction action, KeyCode key, bool replace)
{
	if (replace)
	{
		m_actionBindings[action].clear();
	}
	m_actionBindings[action].push_back(key);
}

void InputMapper::bindAction(GameAction action, const std::vector<KeyCode>& keys, bool replace)
{
	if (replace)
	{
		m_actionBindings[action] = keys;
	}
	else
	{
		auto& bindings = m_actionBindings[action];
		bindings.insert(bindings.end(), keys.begin(), keys.end());
	}
}

void InputMapper::unbindAction(GameAction action)
{
	m_actionBindings.erase(action);
}

void InputMapper::bindAxis(GameAxis axis, const AxisBinding& binding)
{
	m_axisBindings[axis] = binding;
}

void InputMapper::unbindAxis(GameAxis axis)
{
	m_axisBindings.erase(axis);
}

std::vector<KeyCode> InputMapper::getActionBindings(GameAction action) const
{
	auto it = m_actionBindings.find(action);
	if (it != m_actionBindings.end())
	{
		return it->second;
	}
	return std::vector<KeyCode>();
}

const AxisBinding* InputMapper::getAxisBinding(GameAxis axis) const
{
	auto it = m_axisBindings.find(axis);
	if (it != m_axisBindings.end())
	{
		return &it->second;
	}
	return nullptr;
}

// 配置加载/保存


void InputMapper::loadDefaultBindings()
{
	// 清空现有绑定
	m_actionBindings.clear();
	m_axisBindings.clear();

	
	// 动作绑定（离散按键）
	

	// 移动控制（也可以通过轴实现）
	bindAction(GameAction::MoveForward, KeyCode::W);
	bindAction(GameAction::MoveBackward, KeyCode::S);
	bindAction(GameAction::TurnLeft, KeyCode::A);
	bindAction(GameAction::TurnRight, KeyCode::D);
    bindAction(GameAction::Jump, KeyCode::Space);

	// 战斗控制
	bindAction(GameAction::Fire, KeyCode::MouseLeft);
	bindAction(GameAction::AimMode, KeyCode::MouseRight);
	bindAction(GameAction::Reload, KeyCode::R);

	// 相机控制
	bindAction(GameAction::CameraZoomIn, KeyCode::E);
	bindAction(GameAction::CameraZoomOut, KeyCode::Q);

	// UI和系统
	bindAction(GameAction::Pause, KeyCode::P);
	bindAction(GameAction::Menu, KeyCode::Escape);
	bindAction(GameAction::Quit, KeyCode::Escape);
	bindAction(GameAction::Interact, KeyCode::F);

	// 特殊功能
	bindAction(GameAction::ToggleDebugView, KeyCode::F3);
	bindAction(GameAction::Screenshot, KeyCode::F12);

	
	// 轴绑定（连续输入）

	// 移动轴：W(+1) / S(-1)
	bindAxis(GameAxis::MoveVertical, AxisBinding(
		KeyCode::W,      // 正向键（前进）
		KeyCode::S,      // 负向键（后退）
		0.15f,           // 死区
		1.0f             // 灵敏度
	));

	// 转向轴：D(+1) / A(-1)
	bindAxis(GameAxis::TurnHorizontal, AxisBinding(
		KeyCode::D,      // 正向键（右转）
		KeyCode::A,      // 负向键（左转）
		0.15f,           // 死区
		1.0f             // 灵敏度
	));

	// 瞄准轴（鼠标或方向键）
	bindAxis(GameAxis::AimHorizontal, AxisBinding(
		KeyCode::Right,  // 右
		KeyCode::Left,   // 左
		0.1f,
		1.0f
	));

	bindAxis(GameAxis::AimVertical, AxisBinding(
		KeyCode::Up,     // 上
		KeyCode::Down,   // 下
		0.1f,
		1.0f
	));

	// 相机缩放轴
	bindAxis(GameAxis::CameraZoom, AxisBinding(
		KeyCode::E,      // 拉近
		KeyCode::Q,      // 拉远
		0.1f,
		1.0f
	));

	printf("InputMapper::loadDefaultBindings - Loaded %zu actions, %zu axes\n",
	       m_actionBindings.size(), m_axisBindings.size());
}

bool InputMapper::loadBindingsFromFile(const char* configPath)
{
	(void)configPath;  // 未使用的参数（功能未实现）
	
	// TODO: 实现JSON配置文件加载
	// 这里可以使用 nlohmann/json 或其他JSON库
	printf("InputMapper::loadBindingsFromFile - Not implemented yet\n");
	return false;
}

bool InputMapper::saveBindingsToFile(const char* configPath)
{
	(void)configPath;  // 未使用的参数（功能未实现）
	
	// TODO: 实现JSON配置文件保存
	printf("InputMapper::saveBindingsToFile - Not implemented yet\n");
	return false;
}

void InputMapper::resetToDefaults()
{
	loadDefaultBindings();
}


// 内部辅助方法


void InputMapper::updateActionStates()
{
	// 遍历所有动作绑定
	for (const auto& pair : m_actionBindings)
	{
		GameAction action = pair.first;
		const std::vector<KeyCode>& keys = pair.second;
		
		size_t index = static_cast<size_t>(action);
		if (index >= m_actionStates.size())
		{
			continue;
		}

		// 只要有一个绑定的按键被按下，动作就激活
		bool isActive = false;
		for (KeyCode key : keys)
		{
            if (key == KeyCode::MouseLeft)
            {
                if (m_pInputSystem->IsMouseButtonPressed(MouseButton::Left))
                {
                    isActive = true;
                    break;
                }
            }
            else if (key == KeyCode::MouseRight)
            {
                if (m_pInputSystem->IsMouseButtonPressed(MouseButton::Right))
                {
                    isActive = true;
                    break;
                }
            }
            else if (key == KeyCode::MouseMiddle)
            {
                if (m_pInputSystem->IsMouseButtonPressed(MouseButton::Middle))
                {
                    isActive = true;
                    break;
                }
            }
            else
            {
                if (m_pInputSystem->IsKeyPressed(key))
                {
                    isActive = true;
                    break;
                }
            }
		}

		m_actionStates[index] = isActive;
	}
}

void InputMapper::updateAxisValues(float deltaTime)
{
	// 遍历所有轴绑定
	for (const auto& pair : m_axisBindings)
	{
		GameAxis axis = pair.first;
		const AxisBinding& binding = pair.second;
		
		size_t index = static_cast<size_t>(axis);
		if (index >= m_axisValues.size())
		{
			continue;
		}

		// 计算原始轴值
		float rawValue = 0.0f;

		// 检查正向和负向按键
		if (m_pInputSystem->IsKeyPressed(binding.positiveKey))
		{
			rawValue += 1.0f;
		}
		if (m_pInputSystem->IsKeyPressed(binding.negativeKey))
		{
			rawValue -= 1.0f;
		}

		// 应用死区
		float processedValue = applyDeadzone(rawValue, binding.deadzone);

		// 应用灵敏度
		processedValue *= binding.sensitivity;

		// 限制范围到 [-1.0, 1.0]
		processedValue = std::max(-1.0f, std::min(1.0f, processedValue));

		// 应用平滑（如果启用）
		if (binding.smoothing)
		{
			processedValue = smoothDamp(m_smoothedAxisValues[index], processedValue, deltaTime);
			m_smoothedAxisValues[index] = processedValue;
		}

		m_axisValues[index] = processedValue;
	}
}

float InputMapper::applyDeadzone(float value, float deadzone) const
{
	// 如果绝对值小于死区，返回0
	if (std::abs(value) < deadzone)
	{
		return 0.0f;
	}

	// 重新映射范围：[deadzone, 1.0] -> [0.0, 1.0]
	// 保持符号
	float sign = (value > 0.0f) ? 1.0f : -1.0f;
	float absValue = std::abs(value);
	
	// 线性重映射
	float remapped = (absValue - deadzone) / (1.0f - deadzone);
	
	return sign * remapped;
}

float InputMapper::smoothDamp(float current, float target, float deltaTime, float speed) const
{
	// 简单的指数平滑
    // 更进一步可以使用smoothdamp算法来考虑速度和时间，但这里我们使用一个简单的插值方法
	float t = 1.0f - std::exp(-speed * deltaTime);
	return current + (target - current) * t;
}

void InputMapper::enableMouseCapture() const
{
    if (m_pInputSystem)
        m_pInputSystem->enableMouseCapture();
}

void InputMapper::disableMouseCapture() const
{
    if (m_pInputSystem)
        m_pInputSystem->disableMouseCapture();
}