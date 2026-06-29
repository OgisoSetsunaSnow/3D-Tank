#include "game/StateNode/PlayerTankStates.h"

#include "game/StateMachine/StateMachine.h"
#include "game/component/CharacterControllerComponent.h"
#include "game/entity/PlayerTankActor.h"
#include "gameframework/InputManager/InputAction.h"
#include "gameframework/InputManager/InputMapper.h"
#include "gameframework/log/LogSystem.h"

namespace Game
{

void PlayerIdleState::onEnter()
{
    LOG_INFO("[PlayerSM]", ">>> [Idle]");
}

void PlayerIdleState::onUpdate(float dt)
{
    (void) dt;

    if (m_machine->getOwner()->getHp() <= 0.f)
    {
        m_machine->changeState<PlayerDeadState>();
        return;
    }

    InputMapper* input = m_machine->getOwner()->getInputMapper();
    if (!input)
        return;

    const float move = input->getAxisValue(GameAxis::MoveVertical);
    const float turn = input->getAxisValue(GameAxis::TurnHorizontal);
    m_machine->getOwner()->turnMouseSprintArm();
    m_machine->getOwner()->followGunBarrel();
    if (input->isActionJustActivated(GameAction::Fire))
    {
        m_machine->getOwner()->getComponent<CharacterControllerComponent>()->shoot();
    }
    if (move != 0.f || turn != 0.f)
    {
        m_machine->changeState<PlayerMovingState>();
    }
}

void PlayerIdleState::onExit()
{
    LOG_INFO("[PlayerSM]", "<<< [Idle]");
}

void PlayerMovingState::onEnter()
{
    LOG_INFO("[PlayerSM]", ">>> [Moving]");
}

void PlayerMovingState::onUpdate(float dt)
{
    if (m_machine->getOwner()->getHp() <= 0.f)
    {
        m_machine->changeState<PlayerDeadState>();
        return;
    }

    InputMapper* input = m_machine->getOwner()->getInputMapper();
    if (!input)
        return;
    m_machine->getOwner()->turnMouseSprintArm();
    m_machine->getOwner()->followGunBarrel();
    const float move = input->getAxisValue(GameAxis::MoveVertical);
    const float turn = input->getAxisValue(GameAxis::TurnHorizontal);
    if (input->isActionJustActivated(GameAction::Fire))
    {
        m_machine->getOwner()->getComponent<CharacterControllerComponent>()->shoot();
    }
    if (move == 0.f && turn == 0.f)
    {
        m_machine->changeState<PlayerIdleState>();
        return;
    }

    if (turn != 0.f)
    {
        m_machine->getOwner()->transform->addRotation(0.f, turn * m_machine->getOwner()->getTurnSpeed() * dt, 0.f);
    }

    if (move != 0.f)
    {
        glm::vec3 velocity = m_machine->getOwner()->transform->getForward() * move * m_machine->getOwner()->getMoveSpeed();
        m_machine->getOwner()->getComponent<Game::CharacterControllerComponent>()->move(velocity, dt);
    }
}

void PlayerMovingState::onExit()
{
    LOG_INFO("[PlayerSM]", "<<< [Moving]");
}

void PlayerDeadState::onEnter()
{
    LOG_INFO("[PlayerSM]", ">>> [Dead]");
}

void PlayerDeadState::onUpdate(float dt)
{
    (void) dt;
}
void PlayerJumpState::onEnter() {}
void PlayerJumpState::onUpdate(float dt)
{
    (void) dt;
}
void PlayerJumpState::onExit() {}
void PlayerFallingState::onEnter() {}
void PlayerFallingState::onUpdate(float dt)
{
    (void) dt;
}
void PlayerFallingState::onExit() {}
}  // namespace Game
