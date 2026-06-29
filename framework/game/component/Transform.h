#pragma once
#include "game/component/Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
namespace Game
{
class Actor;
class Transform
{
public:
    Transform(Actor* owner)
        : localPosition(glm::vec3(0))
        , localRotation(glm::identity<glm::quat>())
        , localScale(glm::vec3(1))
        , owner(owner)
        , mParent(nullptr)
    {
    }

    Transform(Actor* owner, glm::vec3 pos, glm::quat rotation, glm::vec3 scale)
        : localPosition(pos)
        , localRotation(rotation)
        , localScale(scale)
        , owner(owner)
        , mParent(nullptr)
    {
    }

    /**
     * @brief Get the Model Matrix World object
     *
     * @return glm::mat4
     */
    glm::mat4 getModelMatrixWorld() const;

    /**
     * @brief Get the Model Matrix Local object
     *
     * @return glm::mat4
     */
    glm::mat4 getModelMatrixLocal() const;

    /**
     * @brief Get the Local Position object
     *
     * @return glm::vec3
     */
    glm::vec3 getPosition() const;

    /**
     * @brief Get the World Position object
     *
     * @return glm::vec3
     */
    glm::vec3 getWorldPosition() const;

    /**
     * @brief Set the Position object
     *
     * @param pos
     */
    void setPosition(glm::vec3 pos);

    /**
     * @brief Set the Rotation object
     *
     * @param pitchDeg
     * @param yawDeg
     * @param rollDeg
     */
    void setRotation(float pitchDeg, float yawDeg, float rollDeg);

    /**
     * @brief
     *
     * @param pitchDeg
     * @param yawDeg
     * @param rollDeg
     */
    void addRotation(float pitchDeg, float yawDeg, float rollDeg);

    /**
     * @brief Get the Rotation object
     *
     * @return glm::quat
     */
    glm::quat getRotation() const;

    /**
     * @brief Get the World Rotation object
     *
     * @return glm::quat
     */
    glm::quat getWorldRotation() const;

    /**
     * @brief Get the Scale object
     *
     * @return glm::vec3
     */
    glm::vec3 getScale() const;

    /**
     * @brief Get the World Scale object
     *
     * @return glm::vec3
     */
    glm::vec3 getWorldScale() const;

    /**
     * @brief Get the Parent object
     *
     * @return Transform*
     */
    Transform* getParent() const;

    /**
     * @brief Get the world space forward direction
     * @return
     */
    glm::vec3 getForward() const;

    /**
     * @brief Get the world space up direction
     *
     * @return glm::vec3
     */
    glm::vec3 getUp() const;

    /**
     * @brief Get the world space right direction
     *
     * @return glm::vec3
     */
    glm::vec3 getRight() const;


    /**
     * @brief set parent node
     * @param parent parent node
     */
    void setParent(Transform* newParent);

    /**
     * @brief build parent child relationship
     * @param child
     */
    void addChild(Transform* child);

    /**
     * @brief 通过世界坐标来得到局部坐标
     * @param world
     */
    void setLocalFromWorld(glm::mat4 world);

    void setScale(glm::vec3 Scale);

    /**
     * @brief set actor forward to target direction
     *
     * @param target
     */
    void lookAt(const glm::vec3& target);
    const std::vector<Transform*> getChildren() const;

    glm::vec3 localPosition = glm::vec3{0};
    glm::quat localRotation = glm::identity<glm::quat>();
    glm::vec3 localScale = glm::vec3(1);
    Actor* owner;

private:
    friend class Actor;
    Transform* mParent;
    std::vector<Transform*> mChildren;
};
}  // namespace Game
