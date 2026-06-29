#pragma once
#include "game/entity/Actor.h"
namespace Game
{
class SkyBoxActor: public Actor
{
public:
    SkyBoxActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~SkyBoxActor() override;

private:
};
}  // namespace Game
