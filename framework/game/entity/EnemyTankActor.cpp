#include "game/entity/EnemyTankActor.h"

#include "game/StateNode/EnemyTankStates.h"
#include "game/component/CharacterControllerComponent.h"

#include <game/World.h>
namespace Game
{
EnemyTankActor::EnemyTankActor()
{
    m_characterControllerComp = addComponent<CharacterControllerComponent>();
    name = "EnemyTank";
    m_type = ActorType::ENEMY;
}
void EnemyTankActor::init()
{
    TankActor::init();
    m_moveSpeed = 500.f;
    Actor* player = World::sMain.findPlayerActor();
    if (player)
    {
        m_target = player->transform;
    }
    m_stateMachine = std::make_unique<StateMachine<EnemyTankActor>>(this);
    m_stateMachine->addNode<EnemyPatrolState>();
    m_stateMachine->addNode<EnemyAttackState>();
    m_stateMachine->addNode<EnemyDeadState>();
    m_stateMachine->addNode<EnemyChaseState>();
    m_stateMachine->run<EnemyPatrolState>();
}
void EnemyTankActor::tick(const Engineframework::Timestep& time)
{
    TankActor::tick(time);
    if (!m_target)
    {
        return;
    }

    m_stateMachine->update(time.getSeconds());
    if (transform->getPosition().y < -100.f)
    {
        m_hp = 0.0f;
    }
    // Engineframework::GameFramework::sGet().Renderer().drawDebugRange(transform->getWorldPosition(), sAlertDitance);
    // Engineframework::GameFramework::sGet().Renderer().drawDebugRange(transform->getWorldPosition(), sChasingDitance);
}

void EnemyTankActor::onOverlapBegin(CollisionComponent* self, CollisionComponent* other)
{
    (void) self;
    if (other->getOwner()->getType() == ActorType::BULLET)
    {
        takeDemage(30.0f);
    }
}
void EnemyTankActor::aimAtTarget()
{
    if (!m_target)
    {
        return;
    }

    glm::vec3 selfPos = transform->getPosition();
    glm::vec3 targetPos = m_target->getPosition();
    glm::vec3 dir = targetPos - selfPos;
    dir.y = 0.f;

    if (glm::length(dir) < 0.001f)
    {
        return;
    }

    glm::vec3 localDir = glm::inverse(transform->localRotation) * dir;

    float localYaw = glm::degrees(atan2(localDir.x, localDir.z));

    gunBarrelActor_->transform->localRotation = glm::angleAxis(glm::radians(localYaw), glm::vec3(0.f, 1.f, 0.f));
}
void EnemyTankActor::fire()
{
    if (!m_target)
    {
        return;
    }
    glm::vec3 firePos = getShootPos();
    glm::vec3 targetPos = m_target->getPosition();
    glm::vec3 dir = targetPos - firePos;
    dir.y = 0.f;
    glm::vec3 fireDir = glm::normalize(dir);

    m_characterControllerComp->shoot(firePos, fireDir);
}

void EnemyTankActor::moveToPatroTarget(const glm::vec3& targetPoint, float dt)
{
    glm::vec3 selfPos = transform->getPosition();
    glm::vec3 dir = targetPoint - selfPos;
    dir.y = 0.f;

    if (glm::length(dir) > 50.f)
    {
        glm::vec3 normDir = glm::normalize(dir);
        float targetYaw = glm::degrees(atan2(normDir.x, normDir.z));
        turnTowards(targetYaw, dt);

        glm::vec3 velocity = transform->getForward() * getMoveSpeed() * 0.5f;
        getComponent<CharacterControllerComponent>()->move(velocity, dt);
    }
}

float EnemyTankActor::distToPlayer()
{
    Transform* target = getTarget();
    if (!target)
        return 99999.f;
    glm::vec3 dir = target->getPosition() - transform->getPosition();
    dir.y = 0.f;
    return glm::length(dir);
}

void EnemyTankActor::turnTowards(float targetYaw, float dt)
{
    glm::vec3 forward = transform->getForward();
    forward.y = 0.f;
    forward = glm::normalize(forward);

    float currentYaw = glm::degrees(atan2(forward.x, forward.z));

    float diff = targetYaw - currentYaw;
    if (diff > 180.f)
        diff -= 360.f;
    if (diff < -180.f)
        diff += 360.f;

    float step = glm::sign(diff) * glm::min(turnSpeed * dt, glm::abs(diff));
    float newYaw = currentYaw + step;
    transform->setRotation(0.f, newYaw, 0.f);
}
}  // namespace Game