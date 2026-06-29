#include "game/entity/GunBarreActor.h"

#include "game/component/StaticMeshComponent.h"

namespace Game
{
GunBarreActor::GunBarreActor()
{
   addComponent<StaticMeshComponent>("assets/mesh/TankBattery.OBJ", "assets/tex/TankTex.tga");
    name = "GunBarrel";
}

void GunBarreActor::init()
{
    Actor::init();
}

void GunBarreActor::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);

    muzzlePos = transform->getWorldPosition() + 85.f * transform->getUp() + 285.f * transform->getForward();
     //Engineframework::GameFramework::sGet().Renderer().drawCoordinateSystem(muzzlePos,
     //                                                                       transform->getForward(),
     //                                                                       transform->getUp(),
     //                                                                       transform->getRight());
}

glm::vec3 GunBarreActor::getMuzzlePos() const
{
    return muzzlePos;
}

GunBarreActor::~GunBarreActor() {}
}  // namespace Game
