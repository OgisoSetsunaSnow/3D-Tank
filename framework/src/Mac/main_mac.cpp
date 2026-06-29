#include <Platform.h>

#if VTS_PLATFORM_OSX

#include "audio/AudioInterface.h"
#include "game/TankGame.h"
#include "gameframework/GameFramework.h"
#include "render/BMfont.h"
#include "render/RenderInterface.h"

#include <chrono>
#include <fstream>

#include <assert.h>
#include <stdio.h>
using namespace Engineframework;
using namespace Game;
int main(int /*argc*/, char* /*argv*/[])
{
    auto tankGame = std::make_unique<Game::TankGame>();
    GameFramework::sGet().mainLoop(tankGame.get());
}

#endif  // VTS_PLATFORM_OSX
