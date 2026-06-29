#include "RendererManager.h"

#include "../game/component/Camera.h"
#include "gameframework/assets/AssetCommon.h"
#include "gameframework/log/LogSystem.h"
#include "glm/glm.hpp"
#include "render/RenderInterface.h"

#include <algorithm>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

#include <Platform.h>
#include <render/PC/RenderPC.h>

namespace Engineframework
{
void RendererManager::init()
{
    mRenderer = RenderInterface::CreateRender();
    assert(mRenderer != nullptr);
    mDefaultFontTexture = mRenderer->CreateRenderTextureResource();
    LoadTexture(std::filesystem::path("assets/font/bmfont_verdana_size38_0.tga"), mDefaultFontTexture);
    mRenderer->SetDefaultFont(LoadDefaultFont(std::filesystem::path("assets/font/bmfont_verdana_size38.fnt")),
                              mDefaultFontTexture);
    // 准星贴图
    mCrosshairTexture = mRenderer->CreateRenderTextureResource();
    LoadTexture(std::filesystem::path("assets/tex/crosshair.tga"), mCrosshairTexture);
    mRenderer->Init();
    mHitTexture = mRenderer->CreateRenderDefaultTextureResource();
}

void RendererManager::render()
{
    beginFrame();
    // RenderPass 排序

    if (mHitEffectTimer > 0.0f)
    {
        float t = mHitEffectTimer / mHitEffectDuration;
        float alpha = t * mHitEffectMaxAlpha;
        float fullScreenXform[4][4] = {{2, 0, 0, 0}, {0, 2, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
        float uvFull[4] = {0.f, 0.f, 1.f, 1.f};
        float colorRed[4] = {1.0f, 0.0f, 0.0f, alpha};
        mRenderer->DrawQuad2D(fullScreenXform, uvFull, colorRed, mHitTexture);

        mHitEffectTimer -= fpsCounter.getAvdDt();
    }


    std::sort(mRenderQueue.begin(),
              mRenderQueue.end(),
              [](const RenderItem& it1, const RenderItem& it2) { return it1.sortKey.value < it2.sortKey.value; });
    for (const auto& item : mRenderQueue)
    {
        item.renderRes->Render(mRenderer);
    }
    endFrame();
}

void RendererManager::beginFrame()
{
    assert(mRenderer != nullptr);
    mRenderer->BeginFrame();
    // debug viewProject矩阵上传
    auto viewProjectMatrix = Game::Camera::sMain.getProjection() * Game::Camera::sMain.getViewMatrix();
    float vpRaw[4][4];
    memcpy(vpRaw, glm::value_ptr(viewProjectMatrix), sizeof(vpRaw));
    mRenderer->SetViewProjection(vpRaw);
    // FPS 帧率绘制
    mRenderer->DrawText2D(fpsCounter.getText(), fpsXform, fpsColor, 0.06f);
    drawCrossHair();
}

void RendererManager::endFrame()
{
    assert(mRenderer != nullptr);
    mRenderer->EndFrame();
    mRenderQueue.clear();
}

void RendererManager::shutdown()
{
    assert(mRenderer != nullptr);
    mRenderer->Terminate();
    delete mRenderer;
    mRenderer = nullptr;
}

bool RendererManager::shouldQuit() const
{
    return !mRenderer || mRenderer->ShouldQuit();
}

void RendererManager::submit(const RenderItem& item)
{
    mRenderQueue.push_back(item);
}

void RendererManager::drawCoordinateSystem(glm::vec3 pos,
                                           glm::vec3 forward,
                                           glm::vec3 up,
                                           glm::vec3 right,
                                           float axisLength)
{
    float origin[3] = {pos.x, pos.y, pos.z};
    glm::vec3 xEndVec = pos + right * axisLength;
    float xEnd[3] = {xEndVec.x, xEndVec.y, xEndVec.z};
    float colorRed[3] = {1.0f, 0.0f, 0.0f};
    mRenderer->DebugDrawLine(origin, xEnd, colorRed);

    glm::vec3 yEndVec = pos + up * axisLength;
    float yEnd[3] = {yEndVec.x, yEndVec.y, yEndVec.z};
    float colorGreen[3] = {0.0f, 1.0f, 0.0f};
    mRenderer->DebugDrawLine(origin, yEnd, colorGreen);

    glm::vec3 zEndVec = pos + forward * axisLength;
    float zEnd[3] = {zEndVec.x, zEndVec.y, zEndVec.z};
    float colorBlue[3] = {0.0f, 0.0f, 1.0f};
    mRenderer->DebugDrawLine(origin, zEnd, colorBlue);

    float scale[3] = {30.0f, 30.0f, 30.0f};
    float colorWhite[3] = {1.0f, 1.0f, 1.0f};
    mRenderer->DebugDrawSphere(origin, scale, colorWhite);
    mRenderer->DebugDrawSphere(xEnd, scale, colorRed);
    mRenderer->DebugDrawSphere(yEnd, scale, colorGreen);
    mRenderer->DebugDrawSphere(zEnd, scale, colorBlue);
}

void RendererManager::drawAabb(const AABB& worldAabb)
{
    glm::vec3 mn = worldAabb.getMin();
    glm::vec3 mx = worldAabb.getMax();

    float v0[3] = {mn.x, mn.y, mn.z};
    float v1[3] = {mx.x, mn.y, mn.z};
    float v2[3] = {mx.x, mn.y, mx.z};
    float v3[3] = {mn.x, mn.y, mx.z};

    float v4[3] = {mn.x, mx.y, mn.z};
    float v5[3] = {mx.x, mx.y, mn.z};
    float v6[3] = {mx.x, mx.y, mx.z};
    float v7[3] = {mn.x, mx.y, mx.z};

    float color[3] = {0.0f, 1.0f, 0.0f};  // 绿色

    // 底面 4 条
    mRenderer->DebugDrawLine(v0, v1, color);
    mRenderer->DebugDrawLine(v1, v2, color);
    mRenderer->DebugDrawLine(v2, v3, color);
    mRenderer->DebugDrawLine(v3, v0, color);
    // 顶面 4 条
    mRenderer->DebugDrawLine(v4, v5, color);
    mRenderer->DebugDrawLine(v5, v6, color);
    mRenderer->DebugDrawLine(v6, v7, color);
    mRenderer->DebugDrawLine(v7, v4, color);
    // 竖向 4 条
    mRenderer->DebugDrawLine(v0, v4, color);
    mRenderer->DebugDrawLine(v1, v5, color);
    mRenderer->DebugDrawLine(v2, v6, color);
    mRenderer->DebugDrawLine(v3, v7, color);
}

void RendererManager::drawCrossHair()
{
    // 准星绘制
    float crosshairSize = 0.035f;
    float crosshairXform[4][4] = {{crosshairSize, 0, 0, 0}, {0, crosshairSize, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    float uvFull[4] = {0.f, 0.f, 1.f, 1.f};
    float colorWhite[4] = {1.f, 1.f, 1.f, 1.f};
    mRenderer->DrawQuad2D(crosshairXform, uvFull, colorWhite, mCrosshairTexture);
}

void RendererManager::triggerHitEffect(float duration, float maxAlpha)
{
    mHitEffectTimer = duration;
    mHitEffectDuration = duration;
    mHitEffectMaxAlpha = std::max(mHitEffectMaxAlpha, maxAlpha);
}

void RendererManager::drawDebugRange(const glm::vec3& pos, float range)
{
    float posRaw[3] = {pos.x, pos.y, pos.z};
    float scale[3] = {range, range, range};
    float color[3] = {0.0f, 1.0f, 0.0f};  // 绿色
    mRenderer->DebugDrawSphere(posRaw, scale, color);
}

}  // namespace Engineframework