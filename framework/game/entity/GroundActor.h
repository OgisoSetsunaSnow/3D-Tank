#pragma once
#include "game/entity/Actor.h"
namespace Game
{
class GroundActor : public Actor
{
public:
    GroundActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~GroundActor() override;
private:
};

}  // namespace Game