#pragma once
#include "gameframework/editor/FpsCounter.h"
#include "gameframework/timer/timer.h"
#include "manager/AudioManager.h"
#include "manager/RendererManager.h"
#include "render/PC/Window.h"
#include "render/RenderInterface.h"
#include "manager/InputSystemManager.h"
#include "manager/ResourceManager.h"
#include <vector>

namespace Game
{
class IGame;
}

namespace Engineframework
{
class IManager;
class GameFramework
{
public:
    ~GameFramework() = default;
    GameFramework(const GameFramework& other) = delete;
    GameFramework& operator=(const GameFramework& other) = delete;
    static GameFramework& sGet();
    int mainLoop(Game::IGame* game);
    RendererManager& Renderer() { return mRendererManager; }
    AudioManager& Audio() { return mAudioManager; }
    InputSystemManager& Input() { return mInputSystemManager; }
    ResourceManager& Resource() { return mResourceManager; }

private:
    GameFramework();
    void init();
    void shutdown();
    void update(const Timestep& ts);
    void render();

    RendererManager mRendererManager;
    AudioManager mAudioManager;
    InputSystemManager mInputSystemManager;
    ResourceManager mResourceManager;
    Game::IGame* mGame;
    Timer mTimer;
    std::vector<IManager*> m_engineManagers = {
        &mInputSystemManager,
        &mRendererManager,
        &mAudioManager,
        &mResourceManager,
    };
};
}  // namespace Engineframework
