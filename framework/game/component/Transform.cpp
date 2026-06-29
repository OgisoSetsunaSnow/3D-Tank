#include "Transform.h"

#include "game/component/Camera.h"
#include "gameframework/log/LogSystem.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <stack>
#include <unordered_set>
namespace Game
{
glm::mat4 Transform::getModelMatrixWorld() const
{
    if (mParent)
    {
        return mParent->getModelMatrixWorld() * getModelMatrixLocal();
    }
    return getModelMatrixLocal();
}

glm::mat4 Transform::getModelMatrixLocal() const
{
    glm::mat4 translate = glm::translate(glm::mat4(1.0), localPosition);
    glm::mat4 rotate = glm::mat4_cast(localRotation);
    glm::mat4 scale = glm::scale(glm::mat4(1.f), localScale);
    return translate * rotate * scale;
}

glm::vec3 Transform::getPosition() const
{
    return localPosition;
}

glm::vec3 Transform::getWorldPosition() const
{
    glm::mat4 world = getModelMatrixWorld();
    return glm::vec3(world[3]);
}

void Transform::setPosition(glm::vec3 pos)
{
    localPosition = pos;
}
void Transform::setRotation(float pitchDeg, float yawDeg, float rollDeg)
{
    localRotation = glm::quat(glm::vec3(glm::radians(pitchDeg), glm::radians(yawDeg), glm::radians(rollDeg)));
}
void Transform::addRotation(float pitchDeg, float yawDeg, float rollDeg)
{
    glm::quat delta = glm::quat(glm::vec3(glm::radians(pitchDeg), glm::radians(yawDeg), glm::radians(rollDeg)));
    localRotation = glm::normalize(delta * localRotation);
}
glm::quat Transform::getRotation() const
{
    return localRotation;
}

glm::quat Transform::getWorldRotation() const
{
    return glm::quat_cast(glm::mat3(getModelMatrixWorld()));
}

glm::vec3 Transform::getWorldScale() const
{
    glm::mat4 worldMat = getModelMatrixWorld();
    float scaleX = glm::length(glm::vec3(worldMat[0]));
    float scaleY = glm::length(glm::vec3(worldMat[1]));
    float scaleZ = glm::length(glm::vec3(worldMat[2]));
    glm::vec3 scale = glm::vec3(scaleX, scaleY, scaleZ);
    return scale;
}

glm::vec3 Transform::getScale() const
{
    return localScale;
}
Transform* Transform::getParent() const
{
    return mParent;
}

glm::vec3 Transform::getForward() const
{
    glm::mat4 worldMat = getModelMatrixWorld();
    return glm::normalize(glm::vec3(worldMat * glm::vec4(Camera::localForward, 0.f)));
}

glm::vec3 Transform::getUp() const
{
    glm::mat4 worldMat = getModelMatrixWorld();
    return glm::normalize(glm::vec3(worldMat * glm::vec4(Camera::localUp, 0.f)));
}

glm::vec3 Transform::getRight() const
{
    glm::mat4 worldMat = getModelMatrixWorld();
    return glm::normalize(glm::vec3(worldMat * glm::vec4(Camera::localRight, 0.f)));
}

void Transform::setParent(Transform* newParent)
{
    auto word = getModelMatrixWorld();
    if (mParent)
    {
        auto childParentNode = mParent;
        for (auto it = childParentNode->mChildren.begin(); it != childParentNode->mChildren.end(); it++)
        {
            if (*it == this)
            {
                childParentNode->mChildren.erase(it);
                break;
            }
        }
    }
    mParent = newParent;
    setLocalFromWorld(word);
}

void Transform::addChild(Transform* child)
{
    if (child == nullptr)
    {
        LOG_ERROR("[Game]", "cur child transform is nullptr");
        return;
    }

    if (this == child)
    {
        LOG_ERROR("[Geame]", "parent and child not be the same transform");
        return;
    }

    // DFS查询是否重复节点
    std::stack<Transform*> nodeStack;
    std::unordered_set<Transform*> visted;
    Transform* curParent = this;
    nodeStack.push(curParent);
    while (!nodeStack.empty())
    {
        Transform* cur = nodeStack.top();
        nodeStack.pop();
        if (visted.count(cur))
        {
            LOG_ERROR("[Game]", "chile cycle");
            return;
        }
        visted.insert(cur);
        for (const auto& node : cur->mChildren)
        {
            if (node != nullptr)
            {
                if (node == child)
                {
                    return;
                }
                nodeStack.push(node);
            }
        }
    }

    // child 已经有父节点了
    if (child->mParent != nullptr)
    {
        auto childParentNode = child->mParent;
        for (auto it = childParentNode->mChildren.begin(); it != childParentNode->mChildren.end(); it++)
        {
            if (*it == child)
            {
                childParentNode->mChildren.erase(it);
                break;
            }
        }
    }
    child->setParent(this);
    mChildren.push_back(child);
}

void Transform::setLocalFromWorld(glm::mat4 world)
{
    glm::vec3 scale;
    glm::quat rot;
    glm::vec3 pos;
    glm::vec3 skew;
    glm::vec4 persp;
    glm::mat4 local;
    if (mParent)
    {
        local = glm::inverse(getModelMatrixWorld()) * world;
    }
    else
    {
        local = world;
    }
    glm::decompose(local, scale, rot, pos, skew, persp);
    localPosition = pos;
    localRotation = rot;
    localScale = scale;
}

void Transform::setScale(glm::vec3 Scale)
{
    localScale = Scale;
}

const std::vector<Transform*> Transform::getChildren() const
{
    return mChildren;
}

void Transform::lookAt(const glm::vec3& target)
{
    glm::vec3 f = glm::normalize(target - localPosition);
    glm::vec3 worldUp = glm::vec3{0.f, 1.f, 0.f};
    glm::vec3 right = glm::normalize(glm::cross(worldUp, f));
    glm::vec3 up = glm::cross(f, right);
    glm::mat3 rotMat(right, up, f);
    localRotation = glm::normalize(glm::quat_cast(rotMat));
}

};  // namespace Game