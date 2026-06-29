#include "game/entity/TrankTrackR.h"

#include "game/component/CollisionComponent.h"
#include "game/component/StaticMeshComponent.h"

namespace Game
{
TankTrackR::TankTrackR()
{
   addComponent<StaticMeshComponent>("assets/mesh/TankTrack_R.OBJ", "assets/tex/TankTrack.tga");
    // addComponent<CollisionComponent>();
    name = "TrackR";
}

void TankTrackR::init()
{
    Actor::init();
}

void TankTrackR::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
}

TankTrackR::~TankTrackR() {}
}  // namespace Game
