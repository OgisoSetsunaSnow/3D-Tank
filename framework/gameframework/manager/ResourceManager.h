/**
 * @file ResourceManager.h
 * @author
 * @brief 目前是ResourceManager 依赖RenderInterface 接口，AudioInterface 后续考虑改成RenderInterface 和 AudioInterface
 * 依赖ResourceManager
 * @version 0.1
 * @date 2026-06-17
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "audio/AudioInterface.h"
#include "audio/AudioResourceWave.h"
#include "gameframework/IManager.h"
#include "gameframework/assets/AssetCommon.h"
#include "render/RenderInterface.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace Engineframework
{

class ResourceManager : public IManager
{
public:
    void init() override;
    void shutdown() override;

    RenderMeshResource* loadMesh(const std::string& path);
    RenderTextureResource* loadTexture(const std::string& path);
    AudioResourceWave* loadWave(const std::string& path);

private:
    RenderInterface* mRnd = nullptr;
    AudioInterface* mAudio = nullptr;
    std::unordered_map<std::string, RenderMeshResource*> mMeshes;
    std::unordered_map<std::string, RenderTextureResource*> mTextures;
    std::unordered_map<std::string, AudioResourceWave*> mWaveres;
};

}  // namespace Engineframework