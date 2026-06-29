#pragma once
#include "game/StateMachine/StateMachine.h"
#include "game/entity/Actor.h"
#include "game/entity/GunBarreActor.h"
#include "game/entity/TankTrackL.h"
#include "game/entity/TrankTrackR.h"
#include "gameframework/GameFramework.h"
#include "gameframework/manager/InputSystemManager.h"
namespace Game
{
class StaticMeshComponent;
class CollisionComponent;
class TankActor : public Actor
{
public:
    TankActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~TankActor() override;

    float getHp() const { return m_hp; }
    float getMaxHp() const { return mMaxHp; }
    float getMoveSpeed() const { return m_moveSpeed; }
    float getTurnSpeed() const { return m_turnSpeed; }

    glm::vec3 getShootPos() const;

    /**
     * @brief 处理受到伤害
     *
     * @param demage
     */
    void takeDemage(float demage);

    bool getIsHurt() const { return mIsHurt; }
    void setHurt(bool hurt) { mIsHurt = hurt; }

protected:
    GunBarreActor* gunBarrelActor_;
    TankTrackR* tankTrackRActor_;
    TankTrackL* tankTrackLActor_;
    float m_hp = 100.f;
    float mMaxHp = 100.0f;
    float mLastHp = 100.0f;  // 被攻击之前的血量
    float m_moveSpeed = 1000.f;
    float m_turnSpeed = 90.f;
    bool mIsHurt = false;

private:
    StaticMeshComponent* m_staticMeshComp = nullptr;
    CollisionComponent* m_collisionComp = nullptr;
};
}  // namespace Game
