#include "game/component/SkyComponent.h"

#include "game/component/Camera.h"
#include "game/entity/Actor.h"
#include "gameframework/GameFramework.h"
#include "gameframework/assets/AssetCommon.h"
#include "gameframework/manager/RendererManager.h"
#include "glm/glm.hpp"

#include <filesystem>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

namespace Game
{
SkyComponent::SkyComponent(Actor* actor, const std::string& texturePath)
    : Component(actor)
    , mTexturePath(texturePath)
{
}
void SkyComponent::onInit()
{
    RenderInterface* render = Engineframework::GameFramework::sGet().Renderer().getRenderInterface();
    auto* textureRes = render->CreateRenderTextureResource();
    LoadTexture(std::filesystem::path(mTexturePath.c_str()), textureRes);
    mSkyboxResource = render->CreateRenderResource();
    mSkyboxResource->MeshResource = render->CreateRenderMeshResource();
    mSkyboxResource->TextureResource = textureRes;
    mSkyboxResource->ShaderResource = render->CreateRenderShaderResource(RenderShaderResource::ShaderType_Skybox);
    mSkyboxResource->EnableDepthTest = true;
    mSkyboxResource->EnableDepthWrite = false;
    mSkyboxResource->EnableBackFaceCull = false;
    mSkyboxResource->MeshResource->LoadMesh_Box(1, 1, 1);
    mSkyboxResource->PrepareRender(render);
}
void SkyComponent::onUpdate(const Engineframework::Timestep& ts)
{
    (void) ts;
    // 传递矩阵到GPU
    glm::mat4 view = Camera::sMain.getViewMatrix();
    glm::mat4 proj = Camera::sMain.getProjection();
    // 去除掉天空盒的平移数据
    glm::mat4 viewRotationOnly = glm::mat4(glm::mat3(view));
    glm::mat4 vp = proj * viewRotationOnly;
    float vpRaw[4][4];
    memcpy(vpRaw, glm::value_ptr(vp), sizeof(vpRaw));
    RenderBufferUtil::CopyShaderMatrix(mSkyboxResource->ConstBufferCPU.WorldViewProj, vpRaw);
    Engineframework::RenderItem item;
    item.renderRes = mSkyboxResource;
    item.sortKey = Engineframework::SortKey::sMake(mPass, 0.0f);  // SKYBOX pass，depth=0
    Engineframework::GameFramework::sGet().Renderer().submit(item);
}
void SkyComponent::onShutdown()
{
    if (mSkyboxResource)
    {
        mSkyboxResource->ExitRender(Engineframework::GameFramework::sGet().Renderer().getRenderInterface(), true);
        mSkyboxResource = nullptr;
    }
}
}  // namespace Game