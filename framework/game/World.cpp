#include "game/World.h"

#include "game/entity/Actor.h"

#include <algorithm>
namespace Game
{
World World::sMain;

World::~World() {}

void World::shutdown()
{
    for (auto* actor : mPendingAdd)
    {
        actor->init();
        mActors.push_back(actor);
    }
    for (auto* actor : mActors)
    {
        if (actor->transform && actor->transform->getParent())
        {
            actor->transform->setParent(nullptr);
        }
    }
    for (auto* actor : mActors)
    {
        delete actor;
    }
    mActors.clear();
}

Actor* World::findPlayerActor() const
{
    for (auto* actor : mActors)
    {
        if (actor->getType() == ActorType::PLAYER)
            return actor;
    }
    for (auto* actor : mPendingAdd)
    {
        if (actor->getType() == ActorType::PLAYER)
            return actor;
    }
    return nullptr;
}

void World::destroy(Actor* actor)
{
    if (!actor || actor->isDead)
        return;
    actor->isDead = true;
    mPendingKill.push_back(actor);
}

void World::destroy(Actor* actor, float time)
{
    mGameTimerManager.setTimer(time, [this, actor]() { this->destroy(actor); }, 1);
}

void World::tickAllActor(const Engineframework::Timestep& time)
{
    // 1. 上帧 pending 的 Actor，这帧 init + 加入列表
    mGameTimerManager.tick(time.getSeconds());
    for (auto* actor : mPendingAdd)
    {
        actor->init();
        mActors.push_back(actor);
    }
    mPendingAdd.clear();

    // 2. 正常 Tick
    for (auto& actor : mActors)
    {
        if (!actor->isDead)
        {
            actor->tick(time);
        }
    }
    mPhysicsWorld.update();
    clearDeadActor();
}

void World::clearDeadActor()
{
    if (mPendingKill.empty())
        return;

    for (auto* actor : mPendingKill)
    {
        auto it = std::find(mActors.begin(), mActors.end(), actor);
        if (it != mActors.end())
        {
            mActors.erase(it);
            delete actor;
        }
    }
    mPendingKill.clear();
}
}  // namespace Game
