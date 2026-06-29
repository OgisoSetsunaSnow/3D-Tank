#pragma once
#include "game/StateMachine/StateMachine.h"
#include "game/StateNode/PlayerTankStates.h"
#include "game/entity/TankActor.h"
namespace Game
{
class SpringArmComponent;
class CharacterControllerComponent;
class PlayerTankActor : public TankActor
{
public:
    PlayerTankActor();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~PlayerTankActor() override = default;
    InputMapper* getInputMapper() const { return m_inputMapper; }
    void onChangeMenu();
    void turnMouseSprintArm();
    void followGunBarrel();
    void onOverlapBegin(CollisionComponent* self, CollisionComponent* other) override;

private:
    InputMapper* m_inputMapper = nullptr;
    SpringArmComponent* m_springArm = nullptr;
    CharacterControllerComponent* m_characterControllerComp = nullptr;
    bool m_isMenuMode = false;
    std::unique_ptr<StateMachine<PlayerTankActor>> m_stateMachine;
    static constexpr float sCameraSensitivity = 0.1f;
};
}  // namespace Game