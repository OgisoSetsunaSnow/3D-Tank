#pragma once

#include "IInputSystem.h"
#include "gameframework/InputManager/InputManager.h"

/* 将InputManager适配为IInputSystem接口
 */
class InputManagerAdapter : public IInputSystem
{
public:
	InputManagerAdapter();
	virtual ~InputManagerAdapter();

	// IInputSystem 接口实现
	virtual void Update() override;
	virtual bool IsKeyPressed(KeyCode key) const override;
	virtual bool IsKeyJustPressed(KeyCode key) const override;
	virtual bool IsMouseButtonPressed(MouseButton button) const override;
	virtual bool IsMouseButtonJustPressed(MouseButton button) const override;
	virtual glm::vec2 GetMousePosition() const override;

	virtual glm::vec2 GetMouseDelta() const override;

	virtual void enableMouseCapture() const override;
    virtual void disableMouseCapture() const override;

private:
	InputManager& m_inputManager;
};
