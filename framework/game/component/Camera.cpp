#include "game/component/Camera.h"

namespace Game
{
Camera Camera::sMain{glm::vec3{0.f, 700.f, -1200.f},
                     glm::angleAxis(glm::radians(20.0f), glm::vec3(1.f, 0.f, 0.f)),
                     45.f,
                     1280.f / 720.f,
                     0.1f,
                     10000.f};
glm::vec3 Camera::getForward() const
{
    return glm::normalize(rotation * localForward);
}
glm::vec3 Camera::getRight() const
{
    return glm::normalize(rotation * localRight);
}
glm::vec3 Camera::getUp() const
{
    return glm::normalize(rotation * localUp);
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAtLH(position, position + getForward(), getUp());
}

glm::mat4 Camera::getProjection() const
{
    return glm::perspectiveLH_ZO(glm::radians(mFovYDegrees), mAspectRatio, mNearClip, mFarClip);
}
void Camera::fllowSocket(const glm::vec3& pos, const glm::quat& rot)
{
    position = pos;
    rotation = rot;
}

void Camera::update(float t)
{
    position += localUp * mShake.getOffest(t);
}

void Camera::enableShake(float magnitude)
{
    mShake.trigger();
    mShake.magnitude = magnitude;
}
}  // namespace Game