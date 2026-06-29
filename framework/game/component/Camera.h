#pragma once
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
namespace Game
{
class Transform;
class Camera
{
public:
    static constexpr glm::vec3 localForward = glm::vec3(0.f, 0.f, 1.f);
    static constexpr glm::vec3 localRight = glm::vec3(1.f, 0.f, 0.f);
    static constexpr glm::vec3 localUp = glm::vec3(0.f, 1.f, 0.f);
    Camera() = default;
    Camera(const glm::vec3& pos, const glm::quat& rot, float fov, float aspectRatio, float near, float far)
        : position(pos)
        , rotation(rot)
        , mFovYDegrees(fov)
        , mAspectRatio(aspectRatio)
        , mNearClip(near)
        , mFarClip(far)
    {
    }
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjection() const;


    static Camera sMain;
    // 摄像机的坐标系
    glm::vec3 position = glm::vec3(0.f);
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);

    void fllowSocket(const glm::vec3& pos, const glm::quat& rot);
    void update(float t);
    void enableShake(float magnitude = 2.5f);

private:
    struct CameraShake
    {
        float magnitude = 2.5f;  // 最大偏移幅度
        float frequency = 25.0f;
        float duration = 0.25f;  // 震动持续时间
        float elapsed = 0.0f;
        bool active = false;
        void trigger()
        {
            elapsed = 0.0f;
            active = true;
        }
        float getOffest(float deltaTime)
        {
            if (!active)
                return 0.0f;
            elapsed += deltaTime;
            if (elapsed >= duration)
            {
                active = false;
                elapsed = 0.0f;
                return 0.0f;
            }
            float offest = 0.0f;
            offest = sin(elapsed * frequency) * magnitude * (1.0f - (elapsed / duration));
            return offest;
        }
    };

    float mFovYDegrees = 60.f;
    float mAspectRatio = 16.f / 9.f;
    float mNearClip = 0.1f;
    float mFarClip = 10000.f;
    CameraShake mShake;
};
}  // namespace Game
