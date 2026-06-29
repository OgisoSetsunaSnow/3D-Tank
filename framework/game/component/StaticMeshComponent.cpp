#include "StaticMeshComponent.h"

#include "game/component/Camera.h"
#include "game/entity/Actor.h"
#include "gameframework/GameFramework.h"
#include "gameframework/assets/AssetCommon.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <filesystem>
#include <fstream>
using namespace Engineframework;
namespace Game
{
void StaticMeshComponent::onInit()
{
    RenderInterface* rnd = Engineframework::GameFramework::sGet().Renderer().getRenderInterface();
    RenderMeshResource* meshRes = Engineframework::GameFramework::sGet().Resource().loadMesh(mMeshPath);
    RenderTextureResource* texRes = nullptr;
    if (mTexturePath.empty())
    {
        texRes = rnd->CreateRenderDefaultTextureResource();
    }
    else
    {
        texRes = Engineframework::GameFramework::sGet().Resource().loadTexture(mTexturePath);
    }
    mRenderResource = rnd->CreateRenderResource();
    mRenderResource->MeshResource = meshRes;
    mRenderResource->TextureResource = texRes;
    mRenderResource->ShaderResource = rnd->CreateRenderShaderResource(RenderShaderResource::ShaderType_OpaqueBasic);
    mRenderResource->EnableDepthTest = true;
    mRenderResource->PrepareRender(rnd);
    buildAABB(mRenderResource->MeshResource->VerticesCPU);
}

void StaticMeshComponent::onStart() {}

void StaticMeshComponent::onUpdate(const Engineframework::Timestep& time)
{
    (void) time;
    glm::mat4 mvp = mOwner->getMVP();
    float mvpRaw[4][4];
    memcpy(mvpRaw, glm::value_ptr(mvp), sizeof(mvpRaw));
    RenderBufferUtil::CopyShaderMatrix(mRenderResource->ConstBufferCPU.WorldViewProj, mvpRaw);

    float worldRaw[4][4];
    memcpy(worldRaw, glm::value_ptr(mOwner->transform->getModelMatrixWorld()), sizeof(worldRaw));
    RenderBufferUtil::CopyShaderMatrix(mRenderResource->ConstBufferCPU.World, worldRaw);

    float cameraPosRaw[4];
    memcpy(cameraPosRaw, glm::value_ptr(Camera::sMain.position), sizeof(cameraPosRaw));
    memcpy(mRenderResource->ConstBufferCPU.CameraPos, cameraPosRaw, sizeof(cameraPosRaw));

    RenderItem item;
    item.renderRes = mRenderResource;
    auto cameraDistance = glm::distance(getOwner()->transform->getPosition(), Camera::sMain.position);
    item.sortKey = SortKey::sMake(mPass, cameraDistance);
    Engineframework::GameFramework::sGet().Renderer().submit(item);
}

void StaticMeshComponent::onShutdown()
{
    if (mRenderResource)
    {
        mRenderResource->ExitRender(Engineframework::GameFramework::sGet().Renderer().getRenderInterface(), false);
        mRenderResource = nullptr;
    }
}

void StaticMeshComponent::buildAABB(
    const RenderMeshResource::MeshBufferCPU<RenderMeshResource::MeshVertex>& verticesCPU)
{
    const RenderMeshResource::MeshVertex& vertex0 = verticesCPU.Data[0];
    glm::vec3 minExtent{
        vertex0.position[0],
        vertex0.position[1],
        vertex0.position[2],
    };
    glm::vec3 maxExtent{
        vertex0.position[0],
        vertex0.position[1],
        vertex0.position[2],
    };
    for (int i = 1; i < verticesCPU.NumElements; i++)
    {
        const RenderMeshResource::MeshVertex& vertex = verticesCPU.Data[i];
        minExtent.x = std::min(minExtent.x, vertex.position[0]);
        minExtent.y = std::min(minExtent.y, vertex.position[1]);
        minExtent.z = std::min(minExtent.z, vertex.position[2]);
        maxExtent.x = std::max(maxExtent.x, vertex.position[0]);
        maxExtent.y = std::max(maxExtent.y, vertex.position[1]);
        maxExtent.z = std::max(maxExtent.z, vertex.position[2]);
    }
    mLocalAABB = AABB(minExtent, maxExtent);
}

}  // namespace Game