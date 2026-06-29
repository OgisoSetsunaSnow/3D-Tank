#pragma once
#include "glm/glm.hpp"
namespace Engineframework
{
class AABB
{
public:
    inline AABB(const glm::vec3& minExtents, const glm::vec3& maxExtents)
    {
        mExtents[0] = minExtents;
        mExtents[1] = maxExtents;
    }
    inline AABB() = default;
    ~AABB() = default;

    /**
     * @brief 判断两个AABB是否相交
     *
     * @param other
     * @return true
     * @return false
     */
    inline bool intersects(const AABB& other) const
    {
        if (mExtents[1].x < other.mExtents[0].x || mExtents[0].x > other.mExtents[1].x)
            return false;
        if (mExtents[1].y < other.mExtents[0].y || mExtents[0].y > other.mExtents[1].y)
            return false;
        if (mExtents[1].z < other.mExtents[0].z || mExtents[0].z > other.mExtents[1].z)
            return false;
        return true;
    }

    /**
     * @brief 射线检测 无限长
     *
     * @param start  射线开始的位置
     * @param rayDir 射线的方法 如果传入单位向量，tEnter表示打到AABB时射线的长度
     * @param tEnter 命中AABB的时间
     * @param tExit 离开AABB的时间
     * @return true
     * @return false
     */
    bool intersectRay(const glm::vec3& start, const glm::vec3& rayDir, float& tEnter, float& tExit) const;

    /**
     * @brief 线段检测
     *
     * @param start
     * @param end
     * @return true
     * @return false
     */
    bool intersectLine(const glm::vec3& start, const glm::vec3& end) const;

    /**
     * @brief AABB盒移动
     *
     * @param amt
     * @return AABB
     */
    AABB translate(const glm::vec3& amt) const;

    /**
     * @brief 通过4x4矩阵变换AABB，将8个角点变换后重新计算包围盒
     *        用于从Local AABB计算World AABB
     * @param matrix 世界变换矩阵
     * @return 变换后的AABB（在目标空间中轴对齐）
     */
    AABB transform(const glm::mat4& matrix) const;

    /**
     * @brief 求出重叠区域
     *
     * @param other
     * @return AABB
     */
    AABB oberlap(const AABB& other) const;

    /**
     * @brief 判断点是否在AABB盒内
     *
     * @param point
     * @return true
     * @return false
     */
    bool contains(const glm::vec3& point) const;

    /**
     * @brief 判断AABB other是否完全包含在自身AABB盒当中
     *
     * @param other
     * @return true
     * @return false
     */
    bool contains(const AABB& other) const;

    float getWidth() const;
    float getHeight() const;
    float getDepth() const;
    glm::vec3 getCenter() const;
    glm::vec3 getMin() const { return mExtents[0]; }
    glm::vec3 getMax() const { return mExtents[1]; }

private:
    glm::vec3 mExtents[2];  // mExtents[0]: 包围盒上的最小点
                             // mExtents[1]: 包围盒上的最大点
};
}  // namespace Engineframework