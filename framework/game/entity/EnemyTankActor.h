#pragma once
#include "game/StateMachine/StateMachine.h"
#include "game/entity/TankActor.h"

namespace Game
{
class CharacterControllerComponent;

class EnemyTankActor : public TankActor
{
public:
    EnemyTankActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~EnemyTankActor() override = default;

    void fire();
    Transform* getTarget() const { return m_target; }

    void onOverlapBegin(CollisionComponent* self, CollisionComponent* other) override;
    void aimAtTarget();

    /**
     * @brief 移动到巡逻点
     *
     */
    void moveToPatroTarget(const glm::vec3& targetPoint, float dt);

    /**
     * @brief 计算与玩家之间的距离
     *
     * @return float 距离
     */
    float distToPlayer();

    /**
     * @brief 转向目标点
     *
     * @param targetYaw
     * @param dt
     */
    void turnTowards(float targetYaw, float dt);

    // TODO: 后续可以设置为黑板数据共享
    float sAlertDitance = 2000.f;    // 警觉距离，当玩家进入到这个范围内，敌人会主动攻击
    float sChasingDitance = 4000.f;  // 追逐距离，当玩家主动攻击敌人并且在这个范围内敌人进行追击
    float stopDistance = 500.f;
    float turnSpeed = 120.f;

private:
    CharacterControllerComponent* m_characterControllerComp = nullptr;
    std::unique_ptr<StateMachine<EnemyTankActor>> m_stateMachine;
    Transform* m_target = nullptr;
};
}  // namespace Game