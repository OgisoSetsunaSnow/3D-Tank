#pragma once
#include "game/entity/Actor.h"

namespace Game
{
class GunBarreActor : public Actor
{
public:
    GunBarreActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~GunBarreActor() override;
    glm::vec3 getMuzzlePos() const;
private:
    glm::vec3 muzzlePos;
};
}  // namespace Game
