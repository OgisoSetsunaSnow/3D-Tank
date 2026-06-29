#pragma once
#include "../IManager.h"
#include "gameframework/editor/FpsCounter.h"
#include "gameframework/physics/AABB.h"
#include "glm/glm.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
class RenderInterface;
class RenderTextureResource;
class RenderResource;
namespace Engineframework
{
enum class RenderPass : uint8_t
{
    OPAQUE_ACTOR = 0,
    SKYBOX = 1,
    TRANSPARENT = 2,
    UI = 3,
};

struct SortKey
{
    uint64_t value;
    static SortKey sMake(const RenderPass pass, float cameraDistance)
    {
        uint32_t depthBits = 0;
        switch (pass)
        {
        case RenderPass::OPAQUE_ACTOR:
            depthBits = static_cast<uint32_t>(cameraDistance * 1000.0f);
            break;
        case RenderPass::TRANSPARENT:
            depthBits = 0x00FFFFFF - static_cast<uint32_t>(cameraDistance * 1000.0f);
            break;
        case RenderPass::SKYBOX:
            depthBits = 0;
            break;
        case RenderPass::UI:
            depthBits = 0;
            break;
        }

        return SortKey{(static_cast<uint64_t>(pass) << 56) | (static_cast<uint64_t>(depthBits) << 24)};
    }
};

struct RenderItem
{
    SortKey sortKey;
    RenderResource* renderRes;
};


class RendererManager : public IManager
{
public:
    RendererManager() = default;
    ~RendererManager() override = default;

    void init() override;
    void render() override;
    void shutdown() override;
    void beginFrame();
    void endFrame();

    bool shouldQuit() const;
    RenderInterface* getRenderInterface() { return mRenderer; }
    void submit(const RenderItem& item);
    FpsCounter fpsCounter;

    RenderTextureResource* getDefaultTextRes() { return mDefaultFontTexture; }
    void triggerHitEffect(float duration = 0.4f, float maxAlpha = 0.45f);
    // DEBUG
    /**
     * @brief draw coordingate
     *
     * @param pos world pos of a actor
     * @param forward forward direction of a actor
     * @param up up direction of a actor
     * @param right right direction of a right
     * @param axisLength
     */
    void
    drawCoordinateSystem(glm::vec3 pos, glm::vec3 forward, glm::vec3 up, glm::vec3 right, float axisLength = 300.0f);

    void drawAabb(const AABB& worldAabb);

    void drawDebugLine(const glm::vec3& start, const glm::vec3& dir);

    void drawDebugRange(const glm::vec3& pos, float range);
private:
    RenderInterface* mRenderer = nullptr;
    RenderTextureResource* mDefaultFontTexture = nullptr;
    RenderTextureResource* mCrosshairTexture = nullptr;
    std::vector<RenderItem> mRenderQueue;
    RenderTextureResource* mHitTexture = nullptr;
    // 闪屏时间参数
    float mHitEffectTimer = 0.0f;
    float mHitEffectDuration = 0.2f;
    float mHitEffectMaxAlpha = 0.25f;

    // 渲染FPS
    float fpsXform[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {-0.97f, 0.88f, 0, 1}};
    float fpsColor[4] = {1.0f, 1.0f, 0.2f, 1.0f};

    void drawCrossHair();
};
}  // namespace Engineframework
