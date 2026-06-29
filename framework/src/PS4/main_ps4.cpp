#include <Platform.h>

#if VTS_PLATFORM_PS4
#include <fstream>
#include <assert.h>
#include <chrono>
#include <stdio.h>
#include "render/RenderInterface.h"
#include "audio/AudioInterface.h"
#include "render/BMfont.h"
static bool LoadWaveFile(const char* path, AudioResourceWave* audioRes)
{
	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail())
	{
		printf("file not found: %s\n", path);
		assert(false);
		return false;
	}

	bool ret = audioRes->ReadFile(ifs);
	return ret;
}

static bool LoadMesh(const char* path, RenderMeshResource* meshRes)
{
	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail())
	{
		printf("file not found: %s\n", path);
		assert(false);
		return false;
	}
	bool ret = meshRes->LoadMesh(ifs);
	//bool ret = meshRes->LoadMesh_UnitTest();
	meshRes->CullMode = RenderMeshResource::CCW_FRONT;
	return ret;
}

static bool LoadTexture(const char* path, RenderTextureResource* texRes)
{
	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail())
	{
		printf("file not found: %s\n", path);
		assert(false);
		return false;
	}

	bool ret = texRes->LoadTGATexture(ifs);
	return ret;
}

static BMFont* LoadDefaultFont(const char* path)
{
	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail())
	{
		printf("file not found: %s\n", path);
		assert(false);
		return nullptr;
	}

	BMFont* _font = new BMFont();
	_font->LoadFontDescription(ifs);
	return _font;
}

static void UpdateShaderParameters(RenderResource* res)
{
	static int counter = 0;
	counter++;

	float scale = std::abs(std::sin(static_cast<float> (counter) / 640.0f));

	float temp[4][4] = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
	};
	temp[0][0] = scale;
	temp[1][1] = scale;
	temp[2][2] = scale;
	RenderBufferUtil::CopyShaderMatrix(res->ConstBufferCPU.WorldViewProj, temp);
}

int main(int argc, const char* argv[])
{
	AudioInterface* ad = AudioInterface::CreateAudio();
	ad->Init();
	AudioResourceWave* ares1 = AudioInterface::CreateAudioResource();
	AudioResourceWave* ares2 = AudioInterface::CreateAudioResource();
	LoadWaveFile("/app0/framework/assets/sound/air_conditioner_loop_01.wav", ares1);
	LoadWaveFile("/app0/framework/assets/sound/car_land_ground_weak.wav", ares2);
	ares1->WarmUp(ad);
	ares2->WarmUp(ad);
	ares1->Play(ad);
	auto soundStartTime = std::chrono::steady_clock::now();
	bool secondSoundPlayed = false;

	RenderInterface* rd = RenderInterface::CreateRender();
	RenderTextureResource* fontTexture = rd->CreateRenderTextureResource();
	LoadTexture("/app0/framework/assets/font/bmfont_verdana_size38_0.tga", fontTexture);
	rd->SetDefaultFont(LoadDefaultFont("/app0/framework/assets/font/bmfont_verdana_size38.fnt"), fontTexture);
	RenderResource* res = rd->CreateRenderResource();
	res->MeshResource = rd->CreateRenderMeshResource();
	res->TextureResource = rd->CreateRenderTextureResource();
	res->ShaderResource = rd->CreateRenderShaderResource();

	auto cleanup = [&]() {
		if (res)
		{
			delete res->ShaderResource;
			res->ShaderResource = nullptr;
			delete res->TextureResource;
			res->TextureResource = nullptr;
			delete res->MeshResource;
			res->MeshResource = nullptr;
			delete res;
			res = nullptr;
		}

		delete ares2;
		ares2 = nullptr;
		delete ares1;
		ares1 = nullptr;

		if (ad)
		{
			ad->Terminate();
			delete ad;
			ad = nullptr;
		}

		if (rd)
		{
			rd->Terminate();
			delete rd;
			rd = nullptr;
		}
	};

	rd->Init();
	bool isMeshLoaded = LoadMesh("/app0/framework/assets/mesh/plane.obj", res->MeshResource);
	if (!isMeshLoaded)
	{
		cleanup();
		return -1;
	}

	bool isTextureLoaded = LoadTexture("/app0/framework/assets/tex/TankTex.tga", res->TextureResource);
	if (!isTextureLoaded)
	{
		cleanup();
		return -1;
	}

	res->PrepareRender(rd);

	// --- FPS display: ring-buffer average ---
	static const int FPS_SAMPLE_COUNT = 60;
	float  fpsSamples[FPS_SAMPLE_COUNT] = {};
	int    fpsSampleIndex  = 0;
	int    fpsSamplesFilled = 0;
	auto   lastFrameTime  = std::chrono::steady_clock::now();
	char   fpsText[48]    = "FPS: --";

	while (!rd->ShouldQuit())
	{
		// --- per-frame timing ---
		auto  now = std::chrono::steady_clock::now();
		float dt  = std::chrono::duration<float>(now - lastFrameTime).count();
		lastFrameTime = now;
		if (!secondSoundPlayed && std::chrono::duration<float>(now - soundStartTime).count() >= 1.5f)
		{
			ares2->Play(ad);
			secondSoundPlayed = true;
		}
		if (dt > 0.0f)
		{
			fpsSamples[fpsSampleIndex] = dt;
			fpsSampleIndex = (fpsSampleIndex + 1) % FPS_SAMPLE_COUNT;
			if (fpsSamplesFilled < FPS_SAMPLE_COUNT) ++fpsSamplesFilled;
			float sum = 0.0f;
			for (int s = 0; s < fpsSamplesFilled; ++s) sum += fpsSamples[s];
			float avgDt = sum / (float)fpsSamplesFilled;
			snprintf(fpsText, sizeof(fpsText), "FPS: %.0f  (%.2f ms)", 1.0f / avgDt, avgDt * 1000.0f);
		}

		rd->BeginFrame();
		float viewProj[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		};
		rd->SetViewProjection(viewProj);

		UpdateShaderParameters(res);
		res->Render(rd);

		float xform[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0.5f, 0.5f, 0.5f, 1}
		};
		float uvOffsetScale[4] = { 0, 0, 1, 1 };
		float color[4] = { 1, 1, 1, 1 };
		rd->DrawQuad3D(xform, uvOffsetScale, color, res->TextureResource);

		// debug draw
		float debugdraw_worldPos1[3] = { 0.5f, 0.5f, 0.5f };
		float debugdraw_worldPos2[3] = { -0.5f, -0.5f, 0.5f };
		float sphere_scale[3] = { 1, 1, 1 };
		float sphere_colorRGB1[3] = { 0.7f, 0.2f, 0.2f };
		float sphere_colorRGB2[3] = { 0.2f, 0.7f, 0.2f };
		rd->DebugDrawSphere(debugdraw_worldPos1, sphere_scale, sphere_colorRGB1);
		rd->DebugDrawSphere(debugdraw_worldPos2, sphere_scale, sphere_colorRGB2);
		rd->DebugDrawLine(debugdraw_worldPos1, debugdraw_worldPos2, sphere_colorRGB1);

		float xform3d[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{-0.5f, -0.5f, 0.5f, 1}
		};
		float color2[4] = { 0.1f, 0.8f, 0.1f, 1 };
		rd->DrawText3D("Text3D: ab!cDEFGHijklmn(XYZ)Jf", xform3d, color2, 0.1f);

		float xform3d_depthEnable[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{-0.5f, -0.7f, 0.5f, 1}
		};
		rd->DrawText3D("Text3D: ab!cDEFGHijklmn DepthDisable", xform3d_depthEnable, color2, 0.1f, false);

		const char* text2D = "Text2D: ab!cDEFGHijklmn(XYZ)Jf";
		float xform2d[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{-rd->GetTextTotalWidth(text2D, 0.1f)/2, 0.3, 0, 1}
		};
		float color3[4] = { 0.2f, 0.6f, 0.7f, 1 };
		rd->DrawText2D(text2D, xform2d, color3, 0.1f);

		// --- FPS display (top-left corner) ---
		float fpsXform[4][4] = {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{-0.97f, 0.88f, 0, 1}
		};
		float fpsColor[4] = {1.0f, 1.0f, 0.2f, 1.0f};
		rd->DrawText2D(fpsText, fpsXform, fpsColor, 0.06f);

		rd->EndFrame();
	}
	res->ExitRender(rd, true);
	cleanup();

	return 0;
}
#endif