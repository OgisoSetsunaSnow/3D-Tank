#include "game/component/SpringArmComponent.h"

#include "game/entity/Actor.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Game
{

SpringArmComponent::SpringArmComponent(Actor* owner)
    : Component(owner)
{
}

void SpringArmComponent::onUpdate(const Engineframework::Timestep& ts)
{
    float dt = ts.getSeconds();
    if (m_isTankMoving)
    {
        if (m_hasMouseInput)
        {
            m_movingTimer = 0.f;
        }
        else
        {
            m_movingTimer += dt;
            if (m_movingTimer >= returnDelay)
            {
                float diff = 0.f - m_yaw;
                while (diff > 180.f)
                    diff -= 360.f;
                while (diff < -180.f)
                    diff += 360.f;

                float step = diff * (1.f - glm::exp(-returnSpeed * dt));
                m_yaw += step;

                if (glm::abs(m_yaw) < 0.01f)
                    m_yaw = 0.f;
            }
        }
    }
    else
    {
        m_movingTimer = 0.f;
    }
    m_hasMouseInput = false;

    glm::quat localYaw = glm::angleAxis(glm::radians(m_yaw), glm::vec3(0.f, 1.f, 0.f));
    glm::quat localPitch = glm::angleAxis(glm::radians(m_pitch), glm::vec3(1.f, 0.f, 0.f));
    m_socketRot = mOwner->transform->localRotation * localYaw * localPitch;

    glm::vec3 ownerPos = mOwner->transform->getPosition() + glm::vec3(0.f, heightOffset, 0.f);
    m_socketPos = ownerPos + m_socketRot * glm::vec3(0.f, 0.f, -armLength);
}

}  // namespace Game