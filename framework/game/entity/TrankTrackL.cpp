#include "game/entity/TankTrackL.h"

#include "game/component/CollisionComponent.h"
#include "game/component/StaticMeshComponent.h"

namespace Game
{
TankTrackL::TankTrackL()
{
    addComponent<StaticMeshComponent>("assets/mesh/TankTrack_L.OBJ", "assets/tex/TankTrack.tga");
    // addComponent<CollisionComponent>();
    name = "TrackL";
}

void TankTrackL::init()
{
    Actor::init();
}

void TankTrackL::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
}

TankTrackL::~TankTrackL() {}
}  // namespace Game
