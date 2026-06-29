#pragma once

namespace Game {

template <typename TOwner>
class StateMachine;

template <typename TOwner>
class IState
{
public:
    IState() = default;
    virtual ~IState() = default;
    virtual void onCreate(StateMachine<TOwner>* machine) { m_machine = machine; }
    virtual void onEnter() {}
    virtual void onUpdate(float dt) { (void) dt; }
    virtual void onExit() {}

protected:
    StateMachine<TOwner>* m_machine = nullptr;
};
}
