#pragma once
#include "GameFramework.h"
// #include "gameframework/timer/timer.h"
namespace Game
{
class Actor;
class Component
{
public:
    Component(Actor* owner)
        : mOwner(owner)
    {
        (void) owner;
    }
    virtual ~Component() = default;

    /**
     * @brief 初始化Componet自身的数据，在addComponent的时候触发
     *
     */
    virtual void onInit() {}
    /**
     * @brief  在onInit调用后的下一帧触发。可以与其他的Component交互
     *
     */
    virtual void onStart() {}
    virtual void onUpdate(const Engineframework::Timestep& ts) { (void) ts; }
    virtual void onShutdown() {}
    Actor* getOwner() const { return mOwner; }


protected:
    Actor* mOwner;
    bool mStarted = false;
    friend class Actor;
};
}  // namespace Game
