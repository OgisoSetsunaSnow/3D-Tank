#pragma once
#include "game/component/Component.h"
namespace Game
{
class CharacterControllerComponent : public Component
{
public:
    explicit CharacterControllerComponent(Actor* owner)
        : Component(owner)
    {
    }
    ~CharacterControllerComponent() override = default;

    void onUpdate(const Engineframework::Timestep& ts) override;

    /**
     * @brief 移动
     *
     * @param velocity 移动的速度 具有方向
     * @param dt 帧时间
     */
    void move(const glm::vec3& velocity, float dt);  // 移动

    /**
     * @brief 是否在地面上
     */
    bool isGround() const { return mIsGround; }

    void jump();   // 跳跃
    void shoot();  // 射击
    void shoot(const glm::vec3& pos, const glm::vec3& dir);

private:
    bool checkGround();
    void applyGravity(float dt);
    void moveInternal(const glm::vec3& displacement);

    float mVerticalVelocity = 0.f;  // 当前垂直速度
    bool mIsGround = false;

    static constexpr float sGravity = -980.f;              // 重力加速度
    static constexpr float sMaxFallSpeed = -2000.f;        // 最大下落速度
    static constexpr float sGroundCheckDistance = 20.f;    // 地面检测距离
    static constexpr int sMaxDepenetrationIter = 4;        // SAT检测次数
    static constexpr float sDepenetrationEpsilon = 1e-4f;  // 碰撞检测偏移值
};
}  // namespace Game
