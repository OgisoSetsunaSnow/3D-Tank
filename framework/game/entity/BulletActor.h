#pragma once
#include "game/component/CollisionComponent.h"
#include "game/component/StaticMeshComponent.h"
#include "game/entity/Actor.h"
#include "gameframework/GameFramework.h"
namespace Game
{
class BulletActor : public Actor
{
public:
    BulletActor(glm::vec3 initPos, glm::vec3 dir);
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~BulletActor() override;
    void onOverlapBegin(CollisionComponent* self, CollisionComponent* other) override;

private:
    StaticMeshComponent* m_staticMeshComponent = nullptr;
    CollisionComponent* m_collisionComponent = nullptr;
    glm::vec3 m_moveDir;
};
}  // namespace Game
