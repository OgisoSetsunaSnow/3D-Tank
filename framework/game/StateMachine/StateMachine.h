#pragma once
#include "game/StateMachine/IState.h"
#include "gameframework/log/LogSystem.h"
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace Game
{
template <typename TOwner>
class StateMachine
{
public:
    explicit StateMachine(TOwner* owner) : owner_(owner){}

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;


    template <typename TNode>
    void addNode()
    {
        const std::string name = typeid(TNode).name();
        if (nodes_.count(name))
            return;

        auto node = std::make_shared<TNode>();
        node->onCreate(this);
        nodes_[name] = node;

        LOG_INFO("[StateMachine]", "addNode: %s", name.c_str());
    }


    template <typename TNode>
    void run()
    {
        const std::string name = typeid(TNode).name();
        curNode_ = tryGetNode(name);
        preNode_ = curNode_;

        if (!curNode_)
        {
            LOG_ERROR("[StateMachine]", "run: %s not found", name.c_str());
            return;
        }

        curNode_->onEnter();
        LOG_INFO("[StateMachine]", "run: started at %s", name.c_str());
    }


    void update(float dt)
    {
        if (curNode_)
            curNode_->onUpdate(dt);
    }


    template <typename TNode>
    void changeState()
    {
        const std::string name = typeid(TNode).name();
        auto next = tryGetNode(name);

        if (!next || next == curNode_)
            return;

        LOG_INFO("[StateMachine]", "[%s] --> [%s]", curNode_ ? typeid(*curNode_).name() : "None", name.c_str());

        preNode_ = curNode_;
        if (curNode_)
            curNode_->onExit();
        curNode_ = next;
        curNode_->onEnter();
    }

    TOwner* getOwner() const { return owner_; }
    const char* getCurrentState() const { return curNode_ ? typeid(*curNode_).name() : "None"; }
    const char* getPreviousState() const { return preNode_ ? typeid(*preNode_).name() : "None"; }


private:
    std::shared_ptr<IState<TOwner>> tryGetNode(const std::string& name) const
    {
        auto it = nodes_.find(name);
        return it != nodes_.end() ? it->second : nullptr;
    }

    TOwner* owner_;

    std::shared_ptr<IState<TOwner>> curNode_ = nullptr;
    std::shared_ptr<IState<TOwner>> preNode_ = nullptr;

    std::unordered_map<std::string, std::shared_ptr<IState<TOwner>>> nodes_;
};
}