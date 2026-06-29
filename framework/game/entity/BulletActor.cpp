#include "game/entity/BulletActor.h"

#include "game/World.h"

namespace Game
{
BulletActor::BulletActor(glm::vec3 initPos, glm::vec3 dir)
    : m_moveDir(dir)
{
    transform->localPosition = initPos;
    transform->lookAt(initPos + dir);
    m_staticMeshComponent = addComponent<StaticMeshComponent>("assets/mesh/11111.obj");
    m_collisionComponent = addComponent<CollisionComponent>();
    m_collisionComponent->m_BodyType = BodyType::DYNAMIC;
    name = "BulletActor";
    m_type = ActorType::BULLET;
}

void BulletActor::init()
{
    Actor::init();
    transform->setScale(glm::vec3{10.f, 10.f, 10.f});
}

void BulletActor::onOverlapBegin(CollisionComponent* self, CollisionComponent* other)
{
    (void) self;
    (void) other;
    World::sMain.destroy(this);
}

void BulletActor::tick(const Engineframework::Timestep& time)
{
    Actor::tick(time);
    transform->localPosition += m_moveDir * time.getSeconds() * 15000.f;
}

BulletActor::~BulletActor() {}

}  // namespace Game
