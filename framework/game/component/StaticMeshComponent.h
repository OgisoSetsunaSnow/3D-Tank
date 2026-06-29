#pragma once
#include "game/component/Component.h"
#include "gameframework/manager/RendererManager.h"
#include "gameframework/physics/AABB.h"
#include "render/RenderInterface.h"

#include <string>
namespace Engineframework
{
class GameFramework;
}

namespace Game
{
class StaticMesh;
class Actor;
class StaticMeshComponent : public Component
{
public:
    explicit StaticMeshComponent(Actor* owner, const std::string& meshPath)
        : Component(owner)
        , mMeshPath(meshPath)
    {
    }
    explicit StaticMeshComponent(Actor* owner, const std::string& meshPath, const std::string& texturePath)
        : Component(owner)
        , mMeshPath(meshPath)
        , mTexturePath(texturePath)
    {
    }

    Engineframework::AABB getLocalAABB() const
    {
        return mLocalAABB;
    }

    virtual void onInit() override;
    void onStart() override;
    virtual void onUpdate(const Engineframework::Timestep& time) override;
    virtual void onShutdown() override;

private:
    std::string mMeshPath;
    std::string mTexturePath;
    RenderResource* mRenderResource = nullptr;
    Engineframework::RenderPass mPass = Engineframework::RenderPass::OPAQUE_ACTOR;

    Engineframework::AABB mLocalAABB;
    void buildAABB(const RenderMeshResource::MeshBufferCPU<RenderMeshResource::MeshVertex>& verticesCPU);
};
}  // namespace Game
