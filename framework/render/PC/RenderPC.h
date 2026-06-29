#pragma once

#include <Platform.h>

#if VTS_PLATFORM_WINDOWS
#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include <memory>
#include <vector>
#include "render/RenderInterface.h"
#include "gameframework/log/ImGuiManager.h"


class Window;

class NativeDevicePC;
class FrameResourcePC;
class BMFont;
class RenderShaderResourcePC;
///////////////////////////////////////////////////////////////////////////////
class RenderPC : public RenderInterface
{
	friend class RenderResourcePC;
	friend class RenderResourceInstancedPC;
	friend class RenderMeshResourcePC;
	friend class RenderTextureResourcePC;
	friend class RenderShaderResourcePC;
	friend class NativeDevicePC;

public:
	RenderPC (const RenderPC&) = delete;
	RenderPC& operator= (const RenderPC&) = delete;

	RenderPC ();
	virtual ~RenderPC ();

	virtual int Init() override;
	virtual void BeginFrame() override;
	virtual void EndFrame() override;
	virtual void Terminate() override;
	virtual RenderResource* CreateRenderResource() override;
	virtual RenderMeshResource* CreateRenderMeshResource() override;
	virtual RenderTextureResource* CreateRenderTextureResource() override;
    virtual RenderTextureResource* CreateRenderDefaultTextureResource() override;
	virtual RenderShaderResource* CreateRenderShaderResource(RenderShaderResource::ShaderTypes _type) override;
	static const int QUEUE_SLOT_COUNT = 3;

	static const int MAX_NUM_DEBUG_DRAW_LINES = 1024;
	virtual void DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB) override;
	virtual void DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB) override;
	virtual void SetViewProjection(float(&viewProj)[4][4]) override;
	virtual void SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture) override;
	virtual void DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE) override;
	virtual void DrawText3D(const char* text, float(&worldTransform)[4][4], float(&color)[4], float fontScale, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE) override;

	// use 4x4 matrix for 3x3
	// {
	//	scaleRotation,	scaleRotation,	0,				0,
	//	scaleRotation,	scaleRotation,	0,				0,
	//	0,				0,				1,				0,
	//	posX,			posY,			0,				1
	// }
	virtual void DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture) override;
	virtual void DrawText2D(const char* text, float(&clipTransform)[4][4], float(&color)[4], float fontScale) override;
	virtual float GetTextTotalWidth(const char* text, float fontScale) override;
	virtual bool ShouldQuit() override;
protected:
	int GetQueueSlot() const { return currentBackBuffer_; }

	static constexpr int GetQueueSlotCount ()
	{
		return QUEUE_SLOT_COUNT;
	}

	void EnsureAliveThisFrame(Microsoft::WRL::ComPtr<ID3D12Resource>& res);
	void EnsureAliveThisFrame(Microsoft::WRL::ComPtr<ID3DBlob>& res);

	ID3D12GraphicsCommandList* GetDrawCommandList();

	RenderTextureResource* DefaultTexture;
	BMFont* m_DefaultFont = nullptr;
	RenderTextureResource* m_DefaultFontTexture = nullptr;

private:
	enum QuadDrawType
	{
		QuadDraw2D,
		QuadDraw3D_DepthEnable,
		QuadDraw3D_DepthDisable,

		NumQuadDrawTypes
	};

	void CreateAllocatorsAndCommandLists();
	void CreateBuiltInDrawResources();
	size_t SetupDebugDrawSphere();
	size_t SetupDebugDrawLine();
	void FlushEndFrameDraws(ID3D12GraphicsCommandList* drawCommandList);
	void FlushQuadDraws(ID3D12GraphicsCommandList* drawCommandList);
	RenderResourceInstancedPC* GetOrCreateNextRenderResourceInstanced(QuadDrawType drawType, RenderTextureResource* texture);

	NativeDevicePC* m_NativeDevice;
	FrameResourcePC* m_FrameResource;
	int currentBackBuffer_ = 0;

	float ViewProj[4][4] =
	{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	};

	std::vector<DebugDraw::DrawArgs_Line>  DebugDrawLines;
	std::vector<DebugDraw::DrawArgs_Sphere>  DebugDrawSpheres;

	RenderResourcePC* m_DebugDrawLineResource = nullptr;
	RenderResourceInstancedPC* m_DebugDrawSphereResource = nullptr;
	RenderMeshResourcePC* m_QuadMesh = nullptr;
	RenderShaderResourcePC* m_QuadShader[NumQuadDrawTypes] = { nullptr, nullptr };

	std::vector<std::unique_ptr<RenderResourceInstancedPC>> m_QuadDraws[NumQuadDrawTypes][QUEUE_SLOT_COUNT];	// store ptr instead of value to avoid destruct unexpectedly when emplace inside vector

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_EnsureAliveResources;
	std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> m_EnsureAliveBlobs;

	ImGuiManager m_imguiManager;
};
#endif
