#include "game/entity/GroundActor.h"

#include "GroundActor.h"
#include "game/Constants.h"
#include "game/World.h"
#include "game/component/StaticMeshComponent.h"
namespace Game
{
GroundActor::GroundActor()
{
    addComponent<StaticMeshComponent>("assets/mesh/SM_ZPlane_01a.OBJ", "assets/tex/TX_RockyMud_01_ALB.TGA");
    name = "Ground";
    addComponent<CollisionComponent>();
    m_type = ActorType::GROUND;
}

void GroundActor::init()
{
    Actor::init();
    transform->setScale({5, 5, 5});
}


void GroundActor::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
}
GroundActor::~GroundActor() {}
}  // namespace Game