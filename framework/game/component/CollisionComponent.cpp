#include "CollisionComponent.h"

#include "game/World.h"
#include "game/component/StaticMeshComponent.h"
#include "game/entity/Actor.h"
namespace Game
{
void CollisionComponent::onInit() {}
void CollisionComponent::onStart()
{
    auto* staticMeshComp = mOwner->getComponent<StaticMeshComponent>();
    if (staticMeshComp)
    {
        mLocalAABB = mOwner->getComponent<StaticMeshComponent>()->getLocalAABB();
    }
    else
    {
        // 传入一个默认的AABB
        auto actorPos = getOwner()->transform->getWorldPosition();
        glm::vec3 offset{50.0f, 50.0f, 50.0f};
        glm::vec3 minPos = actorPos - offset;
        glm::vec3 maxPos = actorPos + offset;
        mLocalAABB = Engineframework::AABB(minPos, maxPos);
    }
    World::sMain.getPhysicsWorld().registerCollider(this);
}
void CollisionComponent::onUpdate(const Engineframework::Timestep& time)
{
    (void) time;
}
void CollisionComponent::onShutdown()
{
    World::sMain.getPhysicsWorld().unregisterCollider(this);
}

void CollisionComponent::syncWorldAABB()
{
    mWorldAABB = mLocalAABB.transform(mOwner->transform->getModelMatrixWorld());
}

Engineframework::AABB CollisionComponent::getWorldAABB()
{
    return mWorldAABB;
}

void CollisionComponent::onOverlapBegin(CollisionComponent* other)
{
    mOwner->getRootActor()->onOverlapBegin(this, other);
}

void CollisionComponent::onOverlapStay(CollisionComponent* other)
{
    mOwner->getRootActor()->onOverlapStay(this, other);
}

void CollisionComponent::onOverlapEnd(CollisionComponent* other)
{
    mOwner->getRootActor()->onOverlapEnd(this, other);
}

void CollisionComponent::setBodyType(BodyType type)
{
    m_BodyType = type;
}

void CollisionComponent::setLocalAABB(const Engineframework::AABB& aabb)
{
    mLocalAABB = aabb;
}

}  // namespace Game