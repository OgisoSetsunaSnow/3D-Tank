#pragma once
#include "../IManager.h"
#include "gameframework/InputManager/IInputSystem.h"
#include "gameframework/InputManager/InputMapper.h"

namespace Engineframework
{

class InputSystemManager : public IManager
{
public:
    void init() override;
    void update(const Engineframework::Timestep& ts) override;
    void shutdown() override;

    InputMapper* getInputMapper() const { return mInputMapper; }

private:
    IInputSystem* mInputSystem = nullptr;
    InputMapper* mInputMapper = nullptr;
};

}