#include <Platform.h>
#if VTS_PLATFORM_WINDOWS

#include "audio/AudioInterface.h"
#include "game/TankGame.h"
#include "gameframework/GameFramework.h"
#include "glm/glm.hpp"
#include "imgui.h"
#include "render/RenderInterface.h"
#include "render/bmfont.h"

#include <chrono>
#include <fstream>

#include <Windows.h>
#include <assert.h>
#include <stdio.h>
#include <wrl.h>
using namespace Engineframework;
using namespace Game;

int WinMain(_In_ HINSTANCE /* hInstance */,
            _In_opt_ HINSTANCE /* hPrevInstance */,
            _In_ LPSTR /* lpCmdLine */,
            _In_ int /* nCmdShow */)
{
    auto tankGame = std::make_unique<Game::TankGame>();
    GameFramework::sGet().mainLoop(tankGame.get());
}

#endif  // VTS_PLATFORM_WINDOWS