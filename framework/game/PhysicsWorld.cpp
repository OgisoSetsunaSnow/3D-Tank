#include "game/PhysicsWorld.h"

#include "game/component/CollisionComponent.h"
#include "game/entity/Actor.h"
#include "gameframework/log/LogSystem.h"

#include <glm/glm.hpp>
namespace Game
{
void PhysicsWorld::registerCollider(CollisionComponent* collisionComp)
{
    mAllCollisonComp.push_back(collisionComp);
}

void PhysicsWorld::unregisterCollider(CollisionComponent* collisionComp)
{
    auto it = std::find(mAllCollisonComp.begin(), mAllCollisonComp.end(), collisionComp);
    if (it != mAllCollisonComp.end())
    {
        mAllCollisonComp.erase(it);
    }

    // 删除碰撞对中含有这个CollisionComp的碰撞对
    for (auto pairIt = mPreviousPairs.begin(); pairIt != mPreviousPairs.end();)
    {
        if (pairIt->a == collisionComp || pairIt->b == collisionComp)
        {
            pairIt = mPreviousPairs.erase(pairIt);
        }
        else
        {
            ++pairIt;
        }
    }
}

void PhysicsWorld::update()
{
    for (auto* cc : mAllCollisonComp)
    {
        // 更新为worldAABB
        cc->syncWorldAABB();
    }

    // 暴力 O(N²) 两两 AABB 检测
    std::set<CollisionPair> currentPairs;  // 当前帧碰撞对
    size_t n = mAllCollisonComp.size();
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = i + 1; j < n; j++)
        {
            CollisionComponent* a = mAllCollisonComp[i];
            CollisionComponent* b = mAllCollisonComp[j];
            if (a->getOwner()->getRootActor() == b->getOwner()->getRootActor())
            {
                continue;
            }

            if (a->isStatic() && b->isStatic())
            {
                continue;
            }

            if (a->getWorldAABB().intersects(b->getWorldAABB()))
            {
                //                LOG_INFO("[Collision]", "%s collision %s", a->getOwner()->name.c_str(),
                //                b->getOwner()->name.c_str());
                currentPairs.insert(CollisionPair(a, b));
            }
        }
    }

    // 上一帧没有这一帧有那么就是刚开始发生碰撞 Begin
    for (const auto& pair : currentPairs)
    {
        // 这一帧新出现的碰撞对
        if (mPreviousPairs.find(pair) == mPreviousPairs.end())
        {
            pair.a->onOverlapBegin(pair.b);
            pair.b->onOverlapBegin(pair.a);
        }
    }

    // 上一帧有这一帧也有 Stay
    for (const auto& pair : currentPairs)
    {
        if (mPreviousPairs.find(pair) != mPreviousPairs.end())
        {
            pair.a->onOverlapStay(pair.b);
            pair.b->onOverlapStay(pair.a);
        }
    }

    // 上一帧有这一帧没有
    for (const auto& pair : mPreviousPairs)
    {
        if (currentPairs.find(pair) == currentPairs.end())
        {
            pair.a->onOverlapEnd(pair.b);
            pair.b->onOverlapEnd(pair.a);
        }
    }
    mPreviousPairs = std::move(currentPairs);
}

Engineframework::RayHit PhysicsWorld::raycast(const Engineframework::Ray& ray, float distance) const
{
    size_t n = mAllCollisonComp.size();
    Engineframework::RayHit rayHit;
    rayHit.collider = nullptr;
    float maxDist = (distance < 0.f) ? std::numeric_limits<float>::max() : distance;
    for (size_t i = 0; i < n; i++)
    {
        auto* curCollisionComp = mAllCollisonComp[i];
        float t1, t2;
        auto hit = curCollisionComp->getWorldAABB().intersectRay(ray.origin, ray.direction, t1, t2);
        if (!hit)
            continue;

        if (t1 < 0.f)
            continue;

        if (t1 > maxDist)
            continue;
        if (rayHit.collider == nullptr || rayHit.distance > t1)
        {
            rayHit.collider = curCollisionComp;
            rayHit.point = ray.origin + ray.direction * t1;
            rayHit.distance = t1;
        }
    }
    return rayHit;
}
}  // namespace Game
