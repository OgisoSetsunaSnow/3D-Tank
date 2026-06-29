#include "gameframework/manager/ResourceManager.h"

#include "gameframework/GameFramework.h"
#include "gameframework/log/LogSystem.h"

namespace Engineframework
{

void ResourceManager::init()
{
    mRnd = GameFramework::sGet().Renderer().getRenderInterface();
    mAudio = GameFramework::sGet().Audio().getAudioInterface();
    LOG_INFO("[ResourceManager]", "init");
}

void ResourceManager::shutdown()
{
    for (auto& pair : mMeshes)
        delete pair.second;
    mMeshes.clear();

    for (auto& pair : mTextures)
        delete pair.second;
    mTextures.clear();

    for (auto& pair : mWaveres)
        delete pair.second;
    mWaveres.clear();

    LOG_INFO("[ResourceManager]", "shutdown");
}

RenderMeshResource* ResourceManager::loadMesh(const std::string& path)
{
    auto it = mMeshes.find(path);
    if (it != mMeshes.end())
    {
        LOG_INFO("[ResourceManager]", "mesh cache hit: %s", path.c_str());
        return it->second;
    }

    RenderMeshResource* res = mRnd->CreateRenderMeshResource();
    LoadMesh(std::filesystem::path(path), res);
    mMeshes[path] = res;
    LOG_INFO("[ResourceManager]", "mesh loaded: %s", path.c_str());
    return res;
}

RenderTextureResource* ResourceManager::loadTexture(const std::string& path)
{
    auto it = mTextures.find(path);
    if (it != mTextures.end())
    {
        LOG_INFO("[ResourceManager]", "texture cache hit: %s", path.c_str());
        return it->second;
    }

    RenderTextureResource* res = mRnd->CreateRenderTextureResource();
    LoadTexture(std::filesystem::path(path), res);
    mTextures[path] = res;
    LOG_INFO("[ResourceManager]", "texture loaded: %s", path.c_str());
    return res;
}
AudioResourceWave* ResourceManager::loadWave(const std::string& path)
{
    auto it = mWaveres.find(path);
    if (it != mWaveres.end())
    {
        LOG_INFO("[ResourceManager]", "wave cache hit: %s", path.c_str());
        return it->second;
    }

    AudioResourceWave* res = mAudio->CreateAudioResource();
    LoadWaveFile(std::filesystem::path(path), res);
    mWaveres[path] = res;
    LOG_INFO("[ResourceManager]", "wave loaded: %s", path.c_str());
    return res;
}
}  // namespace Engineframework