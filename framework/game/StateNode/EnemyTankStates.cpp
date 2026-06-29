#include "game/StateNode/EnemyTankStates.h"

#include "game/StateMachine/StateMachine.h"
#include "game/World.h"
#include "game/component/CharacterControllerComponent.h"
#include "game/entity/EnemyTankActor.h"
#include "gameframework/log/LogSystem.h"

#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Game
{


/*
巡逻状态, 在这个状态敌人会随机巡逻当前范围
 */
void EnemyPatrolState::onEnter()
{
    LOG_INFO("[EnemySM]", ">>> [Patrol]");
    pickNewPatrolTarget();
    m_patrolTimerHandle =
        World::sMain.getTimeManager().setTimer(patrolInterval, [this]() { pickNewPatrolTarget(); }, -1);
}

void EnemyPatrolState::onUpdate(float dt)
{
    auto* owner = m_machine->getOwner();
    if (owner->getHp() <= 0.f)
    {
        m_machine->changeState<EnemyDeadState>();
        return;
    }

    if (owner->getIsHurt())
    {
        LOG_INFO("[GamePlayer]", "hurt enter chaseState");
        m_machine->changeState<EnemyChaseState>();
        return;
    }

    if (owner->distToPlayer() < owner->sAlertDitance)
    {
        m_machine->changeState<EnemyAttackState>();
        return;
    }

    m_machine->getOwner()->moveToPatroTarget(m_patrolTarget, dt);
}

void EnemyPatrolState::onExit()
{
    LOG_INFO("[EnemySM]", "<<< [Patrol]");
    World::sMain.getTimeManager().destroyTimer(m_patrolTimerHandle);
    m_patrolTimerHandle.invalidate();
}

void EnemyPatrolState::pickNewPatrolTarget()
{
    glm::vec3 selfPos = m_machine->getOwner()->transform->getPosition();
    float angle = ((float) rand() / RAND_MAX) * 360.f;
    float dist = ((float) rand() / RAND_MAX) * patrolRadius + 200.f;
    m_patrolTarget =
        selfPos + glm::vec3(glm::cos(glm::radians(angle)) * dist, 0.f, glm::sin(glm::radians(angle)) * dist);
}

/*
追逐状态，在这个状态下敌人会一直追逐玩家
*/
void EnemyChaseState::onEnter()
{
    LOG_INFO("[EnemySM]", ">>> [Chase]");
    m_machine->getOwner()->setHurt(false);
}

void EnemyChaseState::onUpdate(float dt)
{
    auto* owner = m_machine->getOwner();
    if (owner->getHp() <= 0.f)
    {
        m_machine->changeState<EnemyDeadState>();
        return;
    }

    float dist = owner->distToPlayer();

    if (dist > owner->sChasingDitance)
    {
        m_machine->changeState<EnemyPatrolState>();
        return;
    }

    if (dist < owner->sAlertDitance)
    {
        m_machine->changeState<EnemyAttackState>();
        return;
    }

    Transform* target = owner->getTarget();
    glm::vec3 dir = target->getPosition() - owner->transform->getPosition();
    dir.y = 0.f;
    glm::vec3 normDir = glm::normalize(dir);
    float targetYaw = glm::degrees(atan2(normDir.x, normDir.z));
    owner->turnTowards(targetYaw, dt);

    glm::vec3 velocity = owner->transform->getForward() * owner->getMoveSpeed();
    m_machine->getOwner()->getComponent<CharacterControllerComponent>()->move(velocity, dt);
}

void EnemyChaseState::onExit()
{
    LOG_INFO("[EnemySM]", "<<< [Chase]");
}

/*
攻击状态, 在这个状态敌人会攻击并持续追逐玩家
 */
void EnemyAttackState::onEnter()
{
    LOG_INFO("[EnemySM]", ">>> [Attack]");
    m_fireTimerHandle = World::sMain.getTimeManager().setTimer(2.f, [this]() { m_machine->getOwner()->fire(); }, -1);
}

void EnemyAttackState::onUpdate(float dt)
{
    auto* owner = m_machine->getOwner();
    if (owner->getHp() <= 0.f)
    {
        m_machine->changeState<EnemyDeadState>();
        return;
    }

    float dist = owner->distToPlayer();

    if (dist > owner->sAlertDitance)
    {
        m_machine->changeState<EnemyPatrolState>();
        return;
    }

    owner->aimAtTarget();
    Transform* target = owner->getTarget();
    glm::vec3 dir = target->getPosition() - owner->transform->getPosition();
    dir.y = 0.f;
    glm::vec3 normDir = glm::normalize(dir);
    float targetYaw = glm::degrees(atan2(normDir.x, normDir.z));

    owner->turnTowards(targetYaw, dt);

    if (dist > owner->stopDistance)
    {
        glm::vec3 velocity = owner->transform->getForward() * owner->getMoveSpeed();
        owner->getComponent<CharacterControllerComponent>()->move(velocity, dt);
    }
}

void EnemyAttackState::onExit()
{
    LOG_INFO("[EnemySM]", "<<< [Attack]");
    World::sMain.getTimeManager().destroyTimer(m_fireTimerHandle);
    m_fireTimerHandle.invalidate();
}

/*
敌人死亡状态
 */
void EnemyDeadState::onEnter()
{
    LOG_INFO("[EnemySM]", ">>> [Dead]");
    World::sMain.destroy(m_machine->getOwner());
    auto boomWave =
        Engineframework::GameFramework::sGet().Resource().loadWave("assets/sound/destroyable_wooden_roof_00.wav");
    Engineframework::GameFramework::sGet().Audio().play(boomWave);
}

void EnemyDeadState::onUpdate(float dt)
{
    (void) dt;
}

}  // namespace Game