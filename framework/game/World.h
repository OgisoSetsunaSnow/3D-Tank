#pragma once
#include "game/PhysicsWorld.h"
#include "game/component/Transform.h"
#include "gameframework/timer/timer.h"
#include "game/timer/GameTimerManager.h"
#include "game/Constants.h"

#include <vector>
namespace Game
{
class Actor;
class World
{
public:
    World() = default;
    ~World();

    template <typename T, typename... Args>
    T* spawnActor(Transform* parent, Args&&... args);

    void destroy(Actor* actor);
    void destroy(Actor* actor, float time);
    void tickAllActor(const Engineframework::Timestep& time);
    void shutdown();
    inline PhysicsWorld& getPhysicsWorld() { return mPhysicsWorld; }
    static World sMain;
    Actor* findPlayerActor() const;
    GameTimerManager& getTimeManager() { return mGameTimerManager; }

private:
    void clearDeadActor();
    std::vector<Actor*> mActors;
    std::vector<Actor*> mPendingAdd;
    std::vector<Actor*> mPendingKill;
    PhysicsWorld mPhysicsWorld;
    GameTimerManager mGameTimerManager;
};

template <typename T, typename... Args>
T* World::spawnActor(Transform* parent, Args&&... args)
{
    T* actor = new T(std::forward<Args>(args)...);
    if (parent)
    {
        parent->addChild(actor->transform);
    }
    mPendingAdd.push_back(actor);
    return actor;
}
}  // namespace Game
