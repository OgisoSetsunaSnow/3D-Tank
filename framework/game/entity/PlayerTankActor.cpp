#include "game/entity/PlayerTankActor.h"

#include "game/component/Camera.h"
#include "game/component/CharacterControllerComponent.h"
#include "game/component/SpringArmComponent.h"
#include "gameframework/InputManager/InputAction.h"
#include "gameframework/InputManager/InputMapper.h"
#include "gameframework/log/LogSystem.h"
namespace Game
{

PlayerTankActor::PlayerTankActor()
{
    m_springArm = addComponent<SpringArmComponent>();
    m_characterControllerComp = addComponent<CharacterControllerComponent>();
    m_type = ActorType::PLAYER;
    name = "PlayerTank";
    m_hp = 1000.f;
}

void PlayerTankActor::init()
{
    TankActor::init();
    m_inputMapper = Engineframework::GameFramework::sGet().Input().getInputMapper();
    m_stateMachine = std::make_unique<StateMachine<PlayerTankActor>>(this);
    m_stateMachine->addNode<PlayerIdleState>();
    m_stateMachine->addNode<PlayerMovingState>();
    m_stateMachine->addNode<PlayerDeadState>();
    m_stateMachine->run<PlayerIdleState>();
    transform->setPosition(glm::vec3{0.f, 10.f, 0.f});
    m_moveSpeed = 1200.0f;
    m_turnSpeed = 120.0f;
}

void PlayerTankActor::tick(const Engineframework::Timestep& time)
{
    TankActor::tick(time);

    if (!m_inputMapper)
        return;

    // 菜单模式切换
    onChangeMenu();

    if (!m_springArm || m_isMenuMode)
        return;

    m_stateMachine->update(time);

    if (m_inputMapper->isActionJustActivated(GameAction::Jump))
    {
        m_characterControllerComp->jump();
    }
}

void PlayerTankActor::onChangeMenu()
{
    if (m_inputMapper->isActionJustActivated(GameAction::Menu))
    {
        m_isMenuMode = !m_isMenuMode;
        if (m_isMenuMode)
            m_inputMapper->disableMouseCapture();
        else
            m_inputMapper->enableMouseCapture();
    }
}

void PlayerTankActor::turnMouseSprintArm()
{
    float mouseX = m_inputMapper->getAxisValue(GameAxis::CameraHorizontal);
    float mouseY = m_inputMapper->getAxisValue(GameAxis::CameraVertical);
    m_springArm->addYawInput(mouseX * sCameraSensitivity);
    m_springArm->addPitchInput(mouseY * sCameraSensitivity);

    bool moving = (m_stateMachine->getCurrentState() == typeid(PlayerMovingState).name());
    m_springArm->setMoving(moving);

    Camera::sMain.fllowSocket(m_springArm->getSocketPos(), m_springArm->getSocketRot());
}

void PlayerTankActor::followGunBarrel()
{
    float yaw = m_springArm->getYaw();
    gunBarrelActor_->transform->localRotation = glm::angleAxis(glm::radians(yaw), glm::vec3(0.f, 1.f, 0.f));
}

void PlayerTankActor::onOverlapBegin(CollisionComponent* self, CollisionComponent* other)
{
    (void) self;
    if (other->getOwner()->getType() == ActorType::BULLET)
    {
        if (m_hp > 0)
        {
            takeDemage(20.0f);
            auto shootWave =
                Engineframework::GameFramework::sGet().Resource().loadWave("assets/sound/gear_shift.wav");
            Engineframework::GameFramework::sGet().Audio().play(shootWave);
            Camera::sMain.enableShake(15.5f);
            Engineframework::GameFramework::sGet().Renderer().triggerHitEffect();
        }
    }
}

}  // namespace Game