#pragma once
#include "game/entity/Actor.h"

namespace Game
{
class TankTrackL : public Actor
{
public:
    TankTrackL();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~TankTrackL() override;
};
}  // namespace Game
