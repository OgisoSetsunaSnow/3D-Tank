#include "game/entity/WallActor.h"

#include "game/World.h"

namespace Game
{
WallActor::WallActor()
{
    m_staticMeshComponent =
        addComponent<StaticMeshComponent>("assets/mesh/SM_Crate_01a.OBJ", "assets/tex/TX_Crates_01a_ALB.TGA");
    m_collisionComponent = addComponent<CollisionComponent>();
    name = "WallActor";
}

void WallActor::init()
{
    Actor::init();
    transform->setPosition(glm::vec3{500.f, 100.f, 500.f});
}

void WallActor::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
}

WallActor::~WallActor() {}

}  // namespace Game
