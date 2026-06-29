#include "InputManagerAdapter.h"

InputManagerAdapter::InputManagerAdapter()
	: m_inputManager(InputManager::GetInstance())
{
}

InputManagerAdapter::~InputManagerAdapter()
{
}

void InputManagerAdapter::Update()
{
	m_inputManager.update();
}

bool InputManagerAdapter::IsKeyPressed(KeyCode key) const
{
	return m_inputManager.isKeyPressed(key);
}

bool InputManagerAdapter::IsKeyJustPressed(KeyCode key) const
{
	return m_inputManager.isKeyJustPressed(key);
}

bool InputManagerAdapter::IsMouseButtonPressed(MouseButton button) const
{
	return m_inputManager.isMouseButtonPressed(button);
}

bool InputManagerAdapter::IsMouseButtonJustPressed(MouseButton button) const
{
	return m_inputManager.isMouseButtonJustPressed(button);
}

glm::vec2 InputManagerAdapter::GetMousePosition() const
{
	return m_inputManager.getMousePosition();
}

glm::vec2 InputManagerAdapter::GetMouseDelta() const
{
    return m_inputManager.getMouseDelta();
}

void InputManagerAdapter::enableMouseCapture() const {
    m_inputManager.enableMouseCapture();
}

void InputManagerAdapter::disableMouseCapture() const {
    m_inputManager.disableMouseCapture();
}

// IInputSystem接口的标准实现
IInputSystem* IInputSystem::Create()
{
	return new InputManagerAdapter();
}

void IInputSystem::Destroy(IInputSystem* pSystem)
{
	delete pSystem;
}
