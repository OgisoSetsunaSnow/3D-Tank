#pragma once
#include "game/StateMachine/IState.h"

namespace Game
{
class PlayerTankActor;

class PlayerIdleState : public Game::IState<Game::PlayerTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;
};

class PlayerMovingState : public Game::IState<Game::PlayerTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;
};

class PlayerDeadState : public Game::IState<Game::PlayerTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
};

class PlayerJumpState : public Game::IState<Game::PlayerTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;
};

class PlayerFallingState : public Game::IState<Game::PlayerTankActor>
{
public:
    void onEnter() override;
    void onUpdate(float dt) override;
    void onExit() override;
};
}