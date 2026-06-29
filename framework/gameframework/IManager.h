#pragma once
#include "gameframework/timer/timer.h"
#include "gameframework/timer/timer.h"
namespace Engineframework
{
class IManager
{
public:
    virtual ~IManager() = default;
    virtual void init() = 0;
    virtual void update(const Timestep& time) { (void) time; }
    virtual void render() {}
    virtual void shutdown() = 0;
};
}  // namespace Engineframework