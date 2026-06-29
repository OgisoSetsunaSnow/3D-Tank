#pragma once
#include "game/StateMachine/IState.h"
#include "game/timer/GameTimerManager.h"
#include "glm/glm.hpp"

namespace Game
{
class EnemyTankActor;

class EnemyPatrolState : public IState<EnemyTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;

private:
    /**
     * @brief 随机生成一个巡逻点，移动到巡逻点 
     * 
     */
    void pickNewPatrolTarget();
    glm::vec3 m_patrolTarget = glm::vec3(0.f);
    TimerHandle m_patrolTimerHandle;
    const float patrolInterval = 3.f;
    const float patrolRadius = 500.f; // 巡逻范围
};

class EnemyChaseState : public IState<EnemyTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;
};


class EnemyAttackState : public IState<EnemyTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;

private:
    TimerHandle m_fireTimerHandle;
};

class EnemyDeadState : public IState<EnemyTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
};

}  // namespace Game