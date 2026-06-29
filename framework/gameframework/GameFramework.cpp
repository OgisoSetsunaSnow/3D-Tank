#include "gameframework/GameFramework.h"

#include "GameFramework.h"
#include "IManager.h"
#include "game/IGame.h"
#include "gameframework/InputManager/IInputSystem.h"
#include "gameframework/InputManager/InputMapper.h"
#include "gameframework/assets/AssetCommon.h"
#include "gameframework/log/LogSystem.h"

#include <chrono>
namespace Engineframework
{

GameFramework& GameFramework::sGet()
{
    static GameFramework instance;
    return instance;
}

GameFramework::GameFramework()
    : mGame(nullptr)
    , mTimer()
{
}


void GameFramework::init()
{
    for (IManager* manager : m_engineManagers)
    {
        manager->init();
    }
    mGame->onInit();
}

int GameFramework::mainLoop(Game::IGame* game)
{
    mGame = game;
    init();
    while (!mRendererManager.shouldQuit())
    {
        Timestep ts = mTimer.tick();
        update(ts);
        render();
    }
    shutdown();
    return 1;
}

void GameFramework::update(const Timestep& ts)
{
    for (auto iter = m_engineManagers.rbegin(); iter != m_engineManagers.rend(); ++iter)
    {
        (*iter)->update(ts);
    }
    Renderer().fpsCounter.update(ts);
    mGame->onUpdate(ts);
}

void GameFramework::shutdown()
{
    mGame->onShutdown();
    for (auto iter = m_engineManagers.rbegin(); iter != m_engineManagers.rend(); ++iter)
    {
        (*iter)->shutdown();
    }
    mGame = nullptr;
}

void GameFramework::render()
{
    mRendererManager.render();
}
}  // namespace Engineframework
