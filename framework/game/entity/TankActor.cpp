#include "game/entity/TankActor.h"

#include "TankActor.h"
#include "game/World.h"
#include "game/component/Camera.h"
#include "game/component/CharacterControllerComponent.h"
#include "game/component/StaticMeshComponent.h"
#include "gameframework/InputManager/InputAction.h"
#include "gameframework/log/LogSystem.h"
namespace Game
{
TankActor::TankActor()
{
    m_staticMeshComp = addComponent<StaticMeshComponent>("assets/mesh/TankBody.OBJ", "assets/tex/TankTex.tga");
    m_collisionComp = addComponent<CollisionComponent>();

    gunBarrelActor_ = World::sMain.spawnActor<GunBarreActor>(transform);
    tankTrackLActor_ = World::sMain.spawnActor<TankTrackL>(transform);
    tankTrackRActor_ = World::sMain.spawnActor<TankTrackR>(transform);

    name = "Tank";
    m_collisionComp->setBodyType(BodyType::DYNAMIC);
}

void TankActor::init()
{
    Actor::init();
}

void TankActor::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
}

TankActor::~TankActor() {}

glm::vec3 TankActor::getShootPos() const
{
    return gunBarrelActor_->getMuzzlePos();
}

void TankActor::takeDemage(float demage)
{
    mLastHp = m_hp;
    m_hp -= demage;
    mIsHurt = true;
}

}  // namespace Game
