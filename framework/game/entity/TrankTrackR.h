#pragma once
#include "game/entity/Actor.h"

namespace Game
{
class TankTrackR : public Actor
{
public:
    TankTrackR();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~TankTrackR() override;
};
}  // namespace Game
