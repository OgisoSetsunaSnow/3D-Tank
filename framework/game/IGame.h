#pragma once
#include "gameframework/GameFramework.h"

namespace Engineframework
{
class GameFramework;
class Timestep;
}  // namespace Engineframework

namespace Game
{
class IGame
{
public:
    virtual ~IGame() = default;
    virtual void onInit() = 0;
    virtual void onUpdate(const Engineframework::Timestep& ts) = 0;
    virtual void onShutdown() = 0;
};
};  // namespace Game
