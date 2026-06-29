#pragma once
#include "audio/AudioResourceWave.h"
#include "render/BMfont.h"
#include "render/RenderInterface.h"

#include <fstream>

#include <assert.h>
#include <stdio.h>

// -----------------------------------------------------------------------
// Cross-platform asset path helper
// -----------------------------------------------------------------------
// ALWAYS use forward slashes (/) in asset paths.  Forward slash works on
// every platform supported by this project (Windows, macOS, Linux, PS4).
// std::filesystem::path will also normalize platform-specifically.
//
// DO NOT use backslashes (\) — they fail on POSIX systems (macOS/Linux).
// -----------------------------------------------------------------------

// Default base directory for game assets (relative to working directory).
// All asset paths below are relative to this directory.
constexpr const char* ASSETS_BASE_DIR = "assets";

bool LoadWaveFile(const std::filesystem::path& path, AudioResourceWave* audioRes);

bool LoadMesh(const std::filesystem::path& path, RenderMeshResource* meshRes);

bool LoadTexture(const std::filesystem::path& path, RenderTextureResource* texRes);

BMFont* LoadDefaultFont(const std::filesystem::path& path);

void UpdateShaderParameters(RenderResource* res);