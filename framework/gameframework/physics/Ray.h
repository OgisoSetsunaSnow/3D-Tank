#pragma once
#include <glm/glm.hpp>


namespace Game
{
class CollisionComponent;
};
namespace Engineframework
{

struct Ray
{
    // 原点
    glm::vec3 origin;
    // 方向，单位向量
    glm::vec3 direction;

    /**
     * @brief Construct a new Ray object
     *
     * @param origin 射线的起始位置
     * @param dir 射线的方向
     */
    Ray(const glm::vec3& origin, const glm::vec3& dir)
        : origin(origin)
        , direction(dir)
    {
    }
};

struct RayHit
{
    Game::CollisionComponent* collider = nullptr;
    // 命中点
    glm::vec3 point = {};
    // 射线原点到命中点的距离
    float distance = 0.f;

    /**
     * @brief 射线是否射中了AABB碰撞体
     *
     * @return true
     * @return false
     */
    bool isValid() const { return collider != nullptr; }
};
}  // namespace Engineframework