#pragma once
#include "game/component/Camera.h"
#include "game/component/Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Game
{
class SpringArmComponent : public Component
{
public:
    explicit SpringArmComponent(Actor* owner);

    virtual void onUpdate(const Engineframework::Timestep& ts) override;

    void addYawInput(float deltaDeg)
    {
        m_yaw += deltaDeg;
        m_hasMouseInput = (glm::abs(deltaDeg) > 0.001f);
    }
    void addPitchInput(float deltaDeg) { m_pitch = glm::clamp(m_pitch + deltaDeg, m_pitchMin, m_pitchMax); }
    void setMoving(bool moving) { m_isTankMoving = moving; }

    glm::vec3 getSocketPos() const { return m_socketPos; }
    glm::quat getSocketRot() const { return m_socketRot; }

    float getYaw() const { return m_yaw; }

    float armLength = 800.f;     // 杆长
    float heightOffset = 300.f;  // 相机挂载点高度偏移
    float m_pitchMin = -10.f;    // 最小仰角
    float m_pitchMax = 40.f;     // 最大俯角
    float returnSpeed = 3.f;     // 回正速度
    float returnDelay = 1.5f;    // 移动多少秒后才开始回正

private:
    float m_yaw = 0.f;
    float m_pitch = 20.f;

    bool m_isTankMoving = false;
    float m_movingTimer = 0.f;     // 持续移动且鼠标未动的累计时间
    bool m_hasMouseInput = false;  // 每帧判断鼠标是否有输入

    glm::vec3 m_socketPos = glm::vec3(0.f);
    glm::quat m_socketRot = glm::identity<glm::quat>();
};
}  // namespace Game