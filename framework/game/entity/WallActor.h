#pragma once
#include "game/component/CollisionComponent.h"
#include "game/component/StaticMeshComponent.h"
#include "game/entity/Actor.h"
#include "gameframework/GameFramework.h"
namespace Game
{
class WallActor : public Actor
{
public:
    WallActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~WallActor() override;

private:
    StaticMeshComponent* m_staticMeshComponent = nullptr;
    CollisionComponent* m_collisionComponent = nullptr;
};
}  // namespace Game
