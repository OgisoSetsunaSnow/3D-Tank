#pragma once
#include "gameframework/physics/Ray.h"

#include <set>
#include <vector>
namespace Game
{
class CollisionComponent;
// (A, B) == (B, A)
struct CollisionPair
{
    CollisionComponent* a;
    CollisionComponent* b;
    CollisionPair(CollisionComponent* a_, CollisionComponent* b_)
    {
        if (a_ < b_)
        {
            a = a_;
            b = b_;
        }
        else
        {
            a = b_;
            b = a_;
        }
    }
    bool operator<(const CollisionPair& other) const
    {
        if (a != other.a)
            return a < other.a;
        return b < other.b;
    }
};

class PhysicsWorld
{
public:
    PhysicsWorld() = default;
    ~PhysicsWorld() = default;

    void registerCollider(CollisionComponent* collisionComp);
    void unregisterCollider(CollisionComponent* collisionComp);
    void update();

    /**
     * @brief 得到所有的AABB碰撞盒
     *
     * @return const std::vector<CollisionComponent*>&
     */
    const std::vector<CollisionComponent*>& getAllColliders() const { return mAllCollisonComp; }

    /**
     * @brief 射线检测
     *
     * @param ray 射线
     * @param distance 射线的长度，传入-1表示为无线长度
     * @return RayHit
     */
    Engineframework::RayHit raycast(const Engineframework::Ray& ray, float distance) const;

private:
    std::vector<CollisionComponent*> mAllCollisonComp;
    // 上一帧的碰撞对
    std::set<CollisionPair> mPreviousPairs;
};

}  // namespace Game