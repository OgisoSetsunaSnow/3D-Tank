#include "game/entity/Actor.h"

#include "game/World.h"
#include "game/component/Camera.h"
#include "game/component/Transform.h"
namespace Game
{
Actor::Actor()
{
    transform = new Transform(this);
    name = "Actor";
}

Actor::~Actor()
{
    clearComponents();
    clearDescendents();
    delete transform;
    transform = nullptr;
}

// Actor的init只负责初始化自己的逻辑
void Actor::init() {}

void Actor::tick(const Engineframework::Timestep& time)
{
    for (auto* c : pendingComponents_)
        components_.push_back(c);
    pendingComponents_.clear();

    for (auto* comp : components_)
    {
        if (!comp->mStarted)
        {
            comp->onStart();
            comp->mStarted = true;
        }
    }

    for (const auto& comp : components_)
        comp->onUpdate(time);
}

void Actor::clearComponents()
{
    for (auto* comp : pendingComponents_)
    {
        comp->onShutdown();
        delete comp;
    }
    pendingComponents_.clear();

    for (auto* comp : components_)
    {
        comp->onShutdown();
        delete comp;
    }
    components_.clear();
}

void Actor::clearDescendents()
{
    for (auto& c : transform->mChildren)
    {
        c->mParent = nullptr;
        World::sMain.destroy(c->owner);
    }
    transform->mChildren.clear();
}

glm::mat4 Actor::getMVP()
{
    return Camera::sMain.getProjection() * Camera::sMain.getViewMatrix() * transform->getModelMatrixWorld();
}

Actor* Actor::getRootActor() const
{
    const Transform* t = transform;
    while (t->getParent() != nullptr)
    {
        t = t->getParent();
    }
    return t->owner;
}

}  // namespace Game
