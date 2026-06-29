#include <filesystem>
#include <fstream>
#include "gameframework/assets/AssetCommon.h"
namespace fs = std::filesystem;
bool LoadWaveFile(const fs::path& path, AudioResourceWave* audioRes)
{
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.fail())
    {
        printf("file not found: %s\n", path.string().c_str());
        assert(false);
        return false;
    }
    return audioRes->ReadFile(ifs);
}

bool LoadMesh(const fs::path& path, RenderMeshResource* meshRes)
{
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.fail())
    {
        printf("file not found: %s\n", path.string().c_str());
        assert(false);
        return false;
    }
    bool ret = meshRes->LoadMesh(ifs);
    meshRes->CullMode = RenderMeshResource::CW_FRONT;
    return ret;
}

bool LoadTexture(const fs::path& path, RenderTextureResource* texRes)
{
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.fail())
    {
        printf("file not found: %s\n", path.string().c_str());
        assert(false);
        return false;
    }
    return texRes->LoadTGATexture(ifs);
}

BMFont* LoadDefaultFont(const fs::path& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (ifs.fail())
    {
        printf("file not found: %s\n", path.string().c_str());
        assert(false);
        return nullptr;
    }
    BMFont* _font = new BMFont();
    _font->LoadFontDescription(ifs);
    return _font;
}
