#include "game/component/CharacterControllerComponent.h"

#include "CharacterControllerComponent.h"
#include "game/World.h"
#include "game/component/Camera.h"
#include "game/component/CollisionComponent.h"
#include "game/entity/Actor.h"
#include "game/entity/BulletActor.h"
#include "game/entity/TankActor.h"
#include "gameframework/log/LogSystem.h"
#include "gameframework/physics/Ray.h"
namespace Game
{

void CharacterControllerComponent::onUpdate(const Engineframework::Timestep& ts)
{
    float dt = ts.getSeconds();

    checkGround();
    applyGravity(dt);
    if (!mIsGround && mVerticalVelocity != 0.f)
    {
        glm::vec3 verticalDisp = {0.f, mVerticalVelocity * dt, 0.f};
        moveInternal(verticalDisp);
    }
}

void CharacterControllerComponent::move(const glm::vec3& velocity, float dt)
{
    moveInternal(velocity * dt);
}

void CharacterControllerComponent::moveInternal(const glm::vec3& displacement)
{
    getOwner()->transform->localPosition += displacement;

    CollisionComponent* myCollisionComp = getOwner()->getComponent<CollisionComponent>();
    assert(myCollisionComp != nullptr && "CharacterControllerComponent requires a CollisionComponent!");

    const auto& allColliders = World::sMain.getPhysicsWorld().getAllColliders();

    for (int iter = 0; iter < sMaxDepenetrationIter; ++iter)
    {
        myCollisionComp->syncWorldAABB();
        bool anyCollision = false;

        for (const auto& collider : allColliders)
        {
            if (collider == myCollisionComp)
                continue;
            if (collider->getOwner()->getRootActor() == getOwner()->getRootActor())
                continue;

            auto myAABB = myCollisionComp->getWorldAABB();
            auto otherAABB = collider->getWorldAABB();
            if (!myAABB.intersects(otherAABB))
                continue;

            anyCollision = true;
            auto overlap = myAABB.oberlap(otherAABB);
            float ox = overlap.getWidth();
            float oy = overlap.getHeight();
            float oz = overlap.getDepth();

            glm::vec3 dir = myAABB.getCenter() - otherAABB.getCenter();
            glm::vec3 correction(0.f);

            if (ox <= oy && ox <= oz)
            {
                float sign = (dir.x >= 0.f) ? 1.f : -1.f;
                correction.x = sign * (ox + sDepenetrationEpsilon);
            }
            else if (oy <= ox && oy <= oz)
            {
                float sign = (dir.y >= 0.f) ? 1.f : -1.f;
                correction.y = sign * (oy + sDepenetrationEpsilon);
            }
            else
            {
                float sign = (dir.z >= 0.f) ? 1.f : -1.f;
                correction.z = sign * (oz + sDepenetrationEpsilon);
            }

            getOwner()->transform->localPosition += correction;
            myCollisionComp->syncWorldAABB();
        }

        if (!anyCollision)
            break;
    }
}

void CharacterControllerComponent::jump()
{
    if (!mIsGround)
    {
        return;
    }
    mVerticalVelocity += 980.f;
    mIsGround = false;
}

void CharacterControllerComponent::shoot()
{
    TankActor* tankActor = dynamic_cast<TankActor*>(getOwner());
    glm::vec3 bulletInitPos = tankActor->getShootPos();
    glm::vec3 camForward = Camera::sMain.getForward();
    camForward.y = 0;
    // 生成子弹
    auto bullet = World::sMain.spawnActor<BulletActor>(nullptr, bulletInitPos, camForward);
    auto shootWave = Engineframework::GameFramework::sGet().Resource().loadWave("assets/sound/subsachine_shot_02.wav");
    Engineframework::GameFramework::sGet().Audio().play(shootWave);
    Camera::sMain.enableShake();
    World::sMain.destroy(bullet, 3.f);
}

void CharacterControllerComponent::shoot(const glm::vec3& pos, const glm::vec3& dir)
{
    auto bullet = World::sMain.spawnActor<BulletActor>(nullptr, pos, dir);
    World::sMain.destroy(bullet, 3.f);
}

bool CharacterControllerComponent::checkGround()
{
    if (mVerticalVelocity > 0.f)
    {
        mIsGround = false;
        return false;
    }
    CollisionComponent* myCollisionComp = getOwner()->getComponent<CollisionComponent>();
    glm::vec3 origin = mOwner->transform->getPosition();
    origin.y = myCollisionComp->getWorldAABB().getMin().y;
    glm::vec3 dir = glm::vec3{0.f, -1.f, 0.f};
    Engineframework::Ray ray{origin, dir};
    Engineframework::RayHit hit = World::sMain.getPhysicsWorld().raycast(ray, sGroundCheckDistance);

    float start[3] = {origin.x, origin.y, origin.z};
    float end[3] = {origin.x, origin.y - sGroundCheckDistance, origin.z};
    float color[3] = {0.f, 1.f, 0.f};

    if (hit.isValid() && hit.collider->getOwner()->getType() == ActorType::GROUND)
    {
        end[0] = hit.point.x;
        end[1] = hit.point.y;
        end[2] = hit.point.z;
        color[0] = 1.f;
        color[1] = 0.f;
        color[2] = 0.f;
        mIsGround = true;
        return true;
    }
    mIsGround = false;
    auto* ri = Engineframework::GameFramework::sGet().Renderer().getRenderInterface();
    ri->DebugDrawLine(start, end, color);
    return false;
}

void CharacterControllerComponent::applyGravity(float dt)
{
    if (mIsGround)
    {
        mVerticalVelocity = 0.f;
    }
    else
    {
        mVerticalVelocity += sGravity * dt;
        if (mVerticalVelocity < sMaxFallSpeed)
            mVerticalVelocity = sMaxFallSpeed;
    }
}
}  // namespace Game
