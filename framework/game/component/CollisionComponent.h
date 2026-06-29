#pragma once
#include "game/component/Component.h"
#include "gameframework/physics/AABB.h"
namespace Game
{
enum class BodyType
{
    STATIC,
    DYNAMIC,
    TRIGGER,
};
class CollisionComponent : public Component
{
public:
    explicit CollisionComponent(Actor* owner)
        : Component(owner)
    {
    }
    ~CollisionComponent() override = default;
    void onInit() override;
    void onStart() override;
    void onUpdate(const Engineframework::Timestep& time) override;
    void onShutdown() override;

    /**
     * @brief 修改AABB碰撞盒大小
     *
     * @param aabb
     */
    void setLocalAABB(const Engineframework::AABB& aabb);

    void syncWorldAABB();
    Engineframework::AABB getWorldAABB();

    // 碰撞事件 会通知到Root Actor
    void onOverlapBegin(CollisionComponent* other);
    void onOverlapStay(CollisionComponent* other);
    void onOverlapEnd(CollisionComponent* other);

    void setBodyType(BodyType);
    bool isStatic() { return m_BodyType == BodyType::STATIC; }

    BodyType m_BodyType = BodyType::STATIC;

private:
    //    bool m_isEnable;
    //    bool m_isTrigger;
    Engineframework::AABB mLocalAABB;
    Engineframework::AABB mWorldAABB;
};
}  // namespace Game
