#include "gameframework/manager/InputSystemManager.h"

namespace Engineframework
{

void InputSystemManager::init()
{
    mInputSystem = IInputSystem::Create();
    mInputMapper = new InputMapper();
    mInputMapper->initialize(mInputSystem);
}

void InputSystemManager::update(const Engineframework::Timestep& ts)
{
    if (mInputSystem)
        mInputSystem->Update();
    if (mInputMapper)
        mInputMapper->update(ts.getSeconds());
}

void InputSystemManager::shutdown()
{
    delete mInputMapper;
    mInputMapper = nullptr;
    IInputSystem::Destroy(mInputSystem);
    mInputSystem = nullptr;
}

}