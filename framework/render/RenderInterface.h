#pragma once

#include <stddef.h>
#include <iostream>
#include <algorithm>

class RenderInterface;
class RenderMeshResource;
class RenderTextureResource;
class BMFont;

class RenderShaderResource
{
public:
	enum ShaderTypes
	{
		ShaderType_OpaqueBasic,
		ShaderType_Instance3D,
		ShaderType_Instance2D,
		ShaderType_Skybox,
		ShaderType_TransparentBasic,
		NumShaderTypes,
	};

	RenderShaderResource(ShaderTypes _type) : ShaderType(_type) { }
	virtual ~RenderShaderResource() = default;

	const ShaderTypes ShaderType;
};

class RenderResource
{
public:
	virtual void PrepareRender(RenderInterface* rnd) { (void)rnd; }
	virtual void Render(RenderInterface* rnd) { (void)rnd; }
	virtual void ExitRender(RenderInterface* rnd, bool waitForRenderDone) { (void)rnd; (void)waitForRenderDone; }
	virtual ~RenderResource() { }
	RenderMeshResource* MeshResource = nullptr;
	RenderTextureResource* TextureResource = nullptr;
	RenderShaderResource* ShaderResource = nullptr;

	struct ConstantBuffer
	{
		float WorldViewProj[4][4] =
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		};
		float World[4][4] = 
		{
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		};
		float CameraPos[4] = {0, 0, 0, 1};
		float LightDir[4] = {0.5f, -1.f, 0.3f, 0.f};
		float LightColor[4] = {0.7f, 0.7f, 0.7f, 1};
	};
	ConstantBuffer ConstBufferCPU;

	enum PrimitiveTypes
	{
		Primitive_TriangleList,
		//Primitive_LineStrip,	// n+1 vtx for n lines, similar to TriangleList layout
		Primitive_LineList,		// 2n vtx for n lines, should only be used in debug draw

		NumPrimtiveTypes
	};
	PrimitiveTypes PrimitiveType = Primitive_TriangleList;
	bool IsWireFrameMode = false;
	bool EnableDepthTest = true;
	bool EnableDepthWrite = true;
	bool EnableBackFaceCull = true;
	bool EnableAlphaBlend = false;

protected:
	virtual void WaitForRenderDone(RenderInterface* rnd) { (void)rnd; }
};

namespace RenderBufferUtil
{
	inline void TransposeMatrix(float* dst, float* src)
	{
		for (int n = 0; n < 16; n++)
		{
			int i = n / 4;
			int j = n % 4;
			dst[n] = src[4 * j + i];
		}
	}
	inline void MulMatrix(float* matA, float* matB, float* matOut)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				matOut[j + i * 4] = 0;
				for (int k = 0; k < 4; k++)
				{
					matOut[j + i * 4] += matA[k + i * 4] * matB[j + k * 4];
				}
			}
		}
	}
	void CopyShaderMatrix(float(&dst)[4][4], float(&src)[4][4]);
}

namespace DebugDraw
{
	struct DrawArgs
	{
		float Color[3] = { 1, 1, 1 };
	};

	struct DrawArgs_Line : DrawArgs
	{
		float StartPos[3] = { 0, 0, 0 };
		float EndPos[3] = { 0, 0, 0 };
	};

	struct DrawArgs_Sphere : DrawArgs
	{
		float Pos[3] = { 0, 0, 0 };
		float Scale[3] = { 1, 1, 1 };
	};
}

#define DRAWQUAD3D_DEFAULT_DEPTH_ENABLE true
#define DRAWQUAD2D_DEFAULT_DEPTH_ENABLE false

class RenderInterface
{
public:
	virtual ~RenderInterface() {}
	virtual int Init() { return 0; }
	virtual void BeginFrame() {}
	virtual void EndFrame() {}
	virtual void Terminate() {}
	virtual bool ShouldQuit() { return false; }

	static RenderInterface* CreateRender();
	virtual RenderResource* CreateRenderResource() { return nullptr; }
	virtual RenderMeshResource* CreateRenderMeshResource() { return nullptr; }
	virtual RenderTextureResource* CreateRenderTextureResource() { return nullptr; }
    virtual RenderTextureResource* CreateRenderDefaultTextureResource() { return nullptr; }
	virtual RenderShaderResource* CreateRenderShaderResource(RenderShaderResource::ShaderTypes _type) 
	{ 
		(void)_type;
		return nullptr; 
	}
	RenderShaderResource* CreateRenderShaderResource() 
	{
		return CreateRenderShaderResource(RenderShaderResource::ShaderType_OpaqueBasic);
	}

	virtual void DebugDrawSphere(float* posXYZ, float* scaleXYZ, float* colorRGB) 
	{
		(void)posXYZ; (void)scaleXYZ; (void)colorRGB;
	}
	virtual void DebugDrawLine(float* pos0XYZ, float* pos1XYZ, float* colorRGB) 
	{
		(void)pos0XYZ; (void)pos1XYZ; (void)colorRGB;
	}
	virtual void SetViewProjection(float(&viewProj)[4][4])
	{
		(void)viewProj;
	}
	virtual void SetDefaultFont(BMFont* fontDesc, RenderTextureResource* texture)
	{
		(void)fontDesc; (void)texture;
	}
	virtual void DrawQuad3D(float(&worldTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE)
	{
		(void)worldTransform; (void)uvOffsetScale; (void)color; (void)texture; (void)isDepthEnable;
	}
	virtual void DrawText3D(const char* text, float(&worldTransform)[4][4], float(&color)[4], float fontScale, bool isDepthEnable = DRAWQUAD3D_DEFAULT_DEPTH_ENABLE)
	{
		(void)text; (void)worldTransform; (void)color; (void)fontScale; (void)isDepthEnable;
	}

	// use 4x4 matrix for 3x3
	// {
	//	scaleRotation,	scaleRotation,	0,				0,
	//	scaleRotation,	scaleRotation,	0,				0,
	//	0,				0,				1,				0,
	//	posX,			posY,			0,				1
	// }
	virtual void DrawQuad2D(float(&clipTransform)[4][4], float(&uvOffsetScale)[4], float(&color)[4], RenderTextureResource* texture)
	{
		(void)clipTransform; (void)uvOffsetScale; (void)color; (void)texture;
	}
	virtual void DrawText2D(const char* text, float(&clipTransform)[4][4], float(&color)[4], float fontScale)
	{
		(void)text; (void)clipTransform; (void)color; (void)fontScale;
	}
	virtual float GetTextTotalWidth(const char* text, float fontScale)
	{
		(void)text; (void)fontScale;
		return 0;
	}
};


class NativeDevice
{
public:
	enum DepthZDirection
	{
		FAR_IS_ZERO,	// D3D, PS4, XB1, ReversedZ
		NEAR_IS_ZERO,	// Traditional
	};

	DepthZDirection DepthZDir = NEAR_IS_ZERO;
};

class FrameResource
{

};

class RenderMeshResource
{
public:
	virtual ~RenderMeshResource() = default;

	/**
	 * @brief 加载模型
	 * @param inputStream 输入流
	 * @return 
	 */
	bool LoadMesh(std::istream& inputStream);
	bool LoadMesh_Plane(float quadWidth = 1.0f);
	bool LoadMesh_Sphere(float radius, int xdiv, int ydiv, float tx = 0, float ty = 0, float tz = 0);

	bool LoadMesh_Box(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
	bool LoadMesh_Cylinder(float radius, float height, int segments);

	struct MeshVertex
	{
		float position[3];
		float vtxcolor[3];
		float uv[2];
        float normal[3];
	};

	template<typename ELEMENT_TYPE>
	class MeshBufferCPU final
	{
	public:
		ELEMENT_TYPE* Data = nullptr;
		size_t AllocBytes = 0;
		int NumElements = 0;
		unsigned int GetDataBytes() { return NumElements * sizeof(ELEMENT_TYPE); }
		void Reset()
		{
			if (Data)
			{
				free(Data);
				Data = nullptr;
				NumElements = 0;
				AllocBytes = 0;
			}
		}
		void GrowIfNeeded(int numMoreElements)
		{
			if ((NumElements + 1) * sizeof(ELEMENT_TYPE) >= AllocBytes)
			{
				// Double capacity each time (amortised O(1)), but always grow
				// by at least numMoreElements elements on the first allocation.
				size_t minAlloc = (size_t)numMoreElements * sizeof(ELEMENT_TYPE);
				AllocBytes = AllocBytes > 0 ? std::max(AllocBytes * 2, minAlloc) : minAlloc;
				Data = (ELEMENT_TYPE*)realloc(Data, AllocBytes);
			}
		}

		void Resize(int numElements)
		{
			if (numElements * sizeof(ELEMENT_TYPE) > AllocBytes)
			{
				AllocBytes = numElements * sizeof(ELEMENT_TYPE);
				Data = (ELEMENT_TYPE*)realloc(Data, AllocBytes);
			}
			NumElements = numElements;
		}

		~MeshBufferCPU() { Reset(); }
	};

	enum MeshCullMode
	{
		CCW_FRONT,
		CW_FRONT,
	};

	MeshBufferCPU<MeshVertex> VerticesCPU;
	MeshBufferCPU<int> IndicesCPU;
	MeshCullMode CullMode = CCW_FRONT;
	bool IsDynamicMesh = false;
};


class RenderTextureResource
{
public:
	bool LoadTGATexture(std::istream& inputStream);
	void CreateDefaultTextureData();
	void ResetTextureData();
	virtual ~RenderTextureResource() { ResetTextureData(); }

	template<typename CHANNEL_TYPE, int NUM_CHANNELS>
	struct PixelType
	{
		CHANNEL_TYPE PixelData[NUM_CHANNELS];
		static constexpr unsigned int GetBytesPerPixel() { return sizeof(CHANNEL_TYPE)* NUM_CHANNELS; }
	};
	typedef PixelType<unsigned char, 4> RGBAPixel;

	int					Width = 0;
	int					Height = 0;

protected:
	RGBAPixel*			RGBATextureDataCPU = nullptr;
	int					RGBATextureDataBytes = 0;
};