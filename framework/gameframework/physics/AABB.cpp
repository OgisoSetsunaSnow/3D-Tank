#include "gameframework/physics/AABB.h"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>

namespace Engineframework
{

bool AABB::intersectRay(const glm::vec3& start, const glm::vec3& rayDir, float& point1, float& point2) const
{
    glm::vec3 rdir = 1.0f / rayDir;

    glm::vec3 t1 = (mExtents[0] - start) * rdir;  // 与 min 平面的交点
    glm::vec3 t2 = (mExtents[1] - start) * rdir;  // 与 max 平面的交点

    glm::vec3 tEnter = glm::min(t1, t2);           // 进入时刻
    glm::vec3 tExit = glm::max(t1, t2);            // 离开时刻

    float tMin = glm::compMax(tEnter);
    float tMax = glm::compMin(tExit);
    point1 = tMin;
    point2 = tMax;
    return tMin <= tMax;
}

bool AABB::intersectLine(const glm::vec3& start, const glm::vec3& end) const
{
    float p1, p2;
    glm::vec3 dir = end - start;
    bool intersect = intersectRay(start, glm::normalize(dir), p1, p2);
    return intersect && p1 * p1 < glm::dot(dir, dir);  // 相交，并且交点长度小于线段的长度
}

AABB AABB::translate(const glm::vec3& amt) const
{
    return AABB(mExtents[0] + amt, mExtents[1] + amt);
}

AABB AABB::transform(const glm::mat4& matrix) const
{
    // 提取8个角点
    glm::vec3 corners[8] = {
        glm::vec3(mExtents[0].x, mExtents[0].y, mExtents[0].z),  // min
        glm::vec3(mExtents[1].x, mExtents[0].y, mExtents[0].z),
        glm::vec3(mExtents[0].x, mExtents[1].y, mExtents[0].z),
        glm::vec3(mExtents[1].x, mExtents[1].y, mExtents[0].z),
        glm::vec3(mExtents[0].x, mExtents[0].y, mExtents[1].z),
        glm::vec3(mExtents[1].x, mExtents[0].y, mExtents[1].z),
        glm::vec3(mExtents[0].x, mExtents[1].y, mExtents[1].z),
        glm::vec3(mExtents[1].x, mExtents[1].y, mExtents[1].z),  // max
    };

    // 变换所有角点并计算新的min/max
    glm::vec4 firstTransformed = matrix * glm::vec4(corners[0], 1.0f);
    glm::vec3 newMin(firstTransformed.x, firstTransformed.y, firstTransformed.z);
    glm::vec3 newMax(newMin);
    for (int i = 1; i < 8; i++)
    {
        glm::vec4 transformed = matrix * glm::vec4(corners[i], 1.0f);
        glm::vec3 p(transformed.x, transformed.y, transformed.z);
        newMin = glm::min(newMin, p);
        newMax = glm::max(newMax, p);
    }
    return AABB(newMin, newMax);
}

AABB AABB::oberlap(const AABB& other) const
{
    return AABB(glm::max(mExtents[0], other.mExtents[0]),  // 取较大的下界
                glm::min(mExtents[1], other.mExtents[1])   // 取较小的上界
    );
}

bool AABB::contains(const glm::vec3& point) const
{
    return glm::all(glm::greaterThanEqual(point, mExtents[0])) &&  // point >= min
           glm::all(glm::lessThanEqual(point, mExtents[1]));       // point <= max
}

bool AABB::contains(const AABB& other) const
{
    return glm::all(glm::lessThanEqual(mExtents[0], other.mExtents[0])) &&
           glm::all(glm::greaterThanEqual(mExtents[1], other.mExtents[1]));
}

float AABB::getWidth() const
{
    return mExtents[1].x - mExtents[0].x;
}

float AABB::getHeight() const
{
    return mExtents[1].y - mExtents[0].y;
}

float AABB::getDepth() const
{
    return mExtents[1].z - mExtents[0].z;
}

glm::vec3 AABB::getCenter() const
{
    return (mExtents[1] + mExtents[0]) / 2.f;
}

}  // namespace Engineframework