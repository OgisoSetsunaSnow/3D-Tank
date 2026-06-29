#pragma once
#include "game/component/Component.h"

namespace Game
{
class SkyComponent : public Component
{
public:
    virtual void onInit() override;
    virtual void onUpdate(const Engineframework::Timestep& ts) override;
    virtual void onShutdown() override;
    SkyComponent(Actor* actor, const std::string& texturePath);
    ~SkyComponent() override = default;

private:
    RenderResource* mSkyboxResource = nullptr;
    std::string mTexturePath;
    Engineframework::RenderPass mPass = Engineframework::RenderPass::SKYBOX;
};
}  // namespace Game