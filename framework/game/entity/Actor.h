/**
 * @file Actor.h
 * @author pangzhengliang
 * @brief 在这个项目Actor可以用来充当Component的容器来管理所有Component的生命周期。
 * 现在Actor也可以充当一部分Controller的能力。
 * @version 0.1
 * @date 2026-06-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "GameFramework.h"
#include "game/Constants.h"
#include "game/component/CharacterControllerComponent.h"
#include "game/component/CollisionComponent.h"
#include "game/component/Component.h"
#include "game/component/Transform.h"
#include "timer/timer.h"

#include <glm/glm.hpp>
#include <vector>

#include <gameframework/log/LogSystem.h>
namespace Game
{
class Transform;
class Component;
class CollisionComponent;
class Actor
{
public:
    /**
     * @brief Actor的构造函数，可以在构造函数中Spawn子物体，和addComponent
     *
     */
    Actor();
    Actor(const std::string& name);
    virtual ~Actor();

    /**
     * @brief 初始化Actor自身的数据
     *
     */
    virtual void init();

    /**
     * @brief Actor进行帧循环，此时所有的Component都已经初始化完全了
     *
     * @param time
     */
    virtual void tick(const Engineframework::Timestep& time);

    virtual void onOverlapBegin(CollisionComponent* self, CollisionComponent* other)
    {
        (void) self;
        (void) other;
    };
    virtual void onOverlapStay(CollisionComponent* self, CollisionComponent* other)
    {
        (void) self;
        (void) other;
    };
    virtual void onOverlapEnd(CollisionComponent* self, CollisionComponent* other)
    {
        (void) self;
        (void) other;
    };

    /**
     * @brief 添加Component
     * @tparam ComponentType
     * @tparam ...Args
     * @param ...args
     * @return
     */
    template <typename ComponentType, typename... Args>
    ComponentType* addComponent(Args&&... args);

    template <typename ComponentType>
    ComponentType* getComponent();

    template <typename ComponentType>
    void removeComponent();

    /**
     * @brief 获取MVP矩阵，默认使用sMain相机
     * @return MVP矩阵
     */
    glm::mat4 getMVP();

    /**
     * @brief Get the Root Actor object
     *
     * @return Actor*
     */
    Actor* getRootActor() const;
    ActorType getType() const { return m_type; }
    void setType(ActorType type) { m_type = type; }
    Transform* transform;
    std::string name;
    bool isDead = false;

protected:
    /**
     * @brief 清理自身的所有的Component
     */
    void clearComponents();

    /**
     * @brief 销毁自身子物体
     */
    void clearDescendents();

    std::vector<Component*> components_;
    std::vector<Component*> pendingComponents_;
    ActorType m_type = ActorType::NONE;
};

template <typename ComponentType, typename... Args>
ComponentType* Actor::addComponent(Args&&... args)
{

    static_assert(std::is_base_of_v<Component, ComponentType>, "Component must derive from Base Component");
    Component* comp = new ComponentType(this, std::forward<Args>(args)...);
    comp->onInit();
    pendingComponents_.push_back(comp);
    return static_cast<ComponentType*>(pendingComponents_.back());
}

template <typename ComponentType>
ComponentType* Actor::getComponent()
{
    // 先从已激活的 components_ 中查找
    auto it = std::find_if(components_.begin(),
                           components_.end(),
                           [](Component* c) { return dynamic_cast<ComponentType*>(c) != nullptr; });
    if (it != components_.end())
        return static_cast<ComponentType*>(*it);

    // 再从待激活的 pendingComponents_ 中查找
    auto pendingIt = std::find_if(pendingComponents_.begin(),
                                  pendingComponents_.end(),
                                  [](Component* c) { return dynamic_cast<ComponentType*>(c) != nullptr; });
    if (pendingIt != pendingComponents_.end())
        return static_cast<ComponentType*>(*pendingIt);

    return nullptr;
}

template <typename ComponentType>
void Actor::removeComponent()
{
    auto it = std::find_if(components_.begin(),
                           components_.end(),
                           [](Component* c) { return dynamic_cast<ComponentType*>(c) != nullptr; });

    if (it != components_.end())
    {
        (*it)->onShutdown();
        delete *it;
        components_.erase(it);
    }
}
}  // namespace Game
