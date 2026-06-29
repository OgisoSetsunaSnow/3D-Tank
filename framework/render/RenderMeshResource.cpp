
#include <iostream>
#include <assert.h>
#include <math.h>
#include <map>
#include <tuple>
#include "render/RenderInterface.h"
#include "render/tiny_obj_loader.h"

struct MeshVertexTempStorage
{
	float position[3];
};

struct MeshTexCoordTempStorage
{
	float uv[2];
};

struct MeshLoadingState
{
	MeshLoadingState(RenderMeshResource* _owner) : Owner(_owner) { }
	RenderMeshResource* Owner = nullptr;
	int NumVertexPositionLoaded = 0;
	int NumVertexColorLoaded = 0;
	int NumTexCoordLoaded = 0;
	int NumIndicesLoaded = 0;
    int NumVertexNormalLoaded = 0;
	int NumOutputVertices = 0;
	RenderMeshResource::MeshBufferCPU<MeshVertexTempStorage> VertexPositionTempstorage;
	RenderMeshResource::MeshBufferCPU<MeshVertexTempStorage> VertexNormalTempstorage;
	RenderMeshResource::MeshBufferCPU<MeshTexCoordTempStorage> VertexTexcoordTempstorage;
	// 用 (position, texcoord, normal) 三元组去重，key = (v_idx, vt_idx, vn_idx)
	std::map<std::tuple<int, int, int>, int> VertexRemap;
};

const int MeshLoadingBufferGrowStep = 1024;
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

static int FixupObjIndex(int idx, int n) 
{
	assert(idx != 0);
	if (idx > 0)
		return idx - 1;
	if (idx < 0)
		return n + idx;  // negative value = relative

	return idx;  // never reach here.
}

const float DefaultVtxColor[3] = { 1.0f, 1.0f, 1.0f };

static void LoadVertexPositionCallback(void* userData, float x, float y, float z, float w)
{
	(void)w;

	MeshLoadingState* state = (MeshLoadingState*)userData;
	state->VertexPositionTempstorage.GrowIfNeeded(MeshLoadingBufferGrowStep);

	auto& vtx = state->VertexPositionTempstorage.Data[(state->NumVertexPositionLoaded)++];
	vtx.position[0] = x;
	vtx.position[1] = y;
	vtx.position[2] = z;

	state->VertexPositionTempstorage.NumElements = max(state->VertexPositionTempstorage.NumElements, state->NumVertexPositionLoaded);
	assert(state->VertexPositionTempstorage.NumElements * sizeof(decltype(*state->VertexPositionTempstorage.Data)) <= state->VertexPositionTempstorage.AllocBytes);
}

static void LoadNormalCallback(void* userData, float x, float y, float z)
{
    MeshLoadingState* state = (MeshLoadingState*) userData;
    state->VertexNormalTempstorage.GrowIfNeeded(MeshLoadingBufferGrowStep);

	auto& vtx = state->VertexNormalTempstorage.Data[(state->NumVertexNormalLoaded)++];
    vtx.position[0] = x;
	vtx.position[1] = y;
	vtx.position[2] = z;

	state->VertexNormalTempstorage.NumElements = max(state->VertexNormalTempstorage.NumElements, state->NumVertexNormalLoaded);
    assert(state->VertexNormalTempstorage.NumElements * sizeof(decltype(*state->VertexNormalTempstorage.Data)) <= state->VertexNormalTempstorage.AllocBytes);
}

static void LoadTexCoordCallback(void* userData, float x, float y, float z)
{
	(void)z;

	MeshLoadingState* state = (MeshLoadingState*)userData;
	state->VertexTexcoordTempstorage.GrowIfNeeded(MeshLoadingBufferGrowStep);

	auto& vtx = state->VertexTexcoordTempstorage.Data[(state->NumTexCoordLoaded)++];
	vtx.uv[0] = x;
	vtx.uv[1] = 1.f - y;

	state->VertexTexcoordTempstorage.NumElements = max(state->VertexTexcoordTempstorage.NumElements, state->NumTexCoordLoaded);
	assert(state->VertexTexcoordTempstorage.NumElements * sizeof(decltype(*state->VertexTexcoordTempstorage.Data)) <= state->VertexTexcoordTempstorage.AllocBytes);
}

static void LoadIndexCallback(void* userData, tinyobj::index_t* indices, int numIndices)
{
	MeshLoadingState* state = (MeshLoadingState*)userData;
	assert(numIndices == 3);

	for (int i=0; i < numIndices; i++)
	{
		assert(indices[i].vertex_index > 0);
		assert(indices[i].texcoord_index > 0);

		int vtx_index = FixupObjIndex(indices[i].vertex_index, state->NumVertexPositionLoaded);
		int texcoord_index = FixupObjIndex(indices[i].texcoord_index, state->NumTexCoordLoaded);
		int nor_index = (indices[i].normal_index > 0)
			? FixupObjIndex(indices[i].normal_index, state->NumVertexNormalLoaded)
			: -1;

		auto key = std::make_tuple(vtx_index, texcoord_index, nor_index);
		auto it = state->VertexRemap.find(key);
		int outputIndex;
		if (it != state->VertexRemap.end())
		{
			outputIndex = it->second;
		}
		else
		{
			outputIndex = state->NumOutputVertices++;
			state->VertexRemap[key] = outputIndex;

			state->Owner->VerticesCPU.GrowIfNeeded(MeshLoadingBufferGrowStep);
			RenderMeshResource::MeshVertex& vtx = state->Owner->VerticesCPU.Data[outputIndex];

			// 位置
			assert(vtx_index < state->VertexPositionTempstorage.NumElements);
			memcpy(vtx.position, state->VertexPositionTempstorage.Data[vtx_index].position, sizeof(vtx.position));

			// 颜色
			memcpy(vtx.vtxcolor, DefaultVtxColor, sizeof(DefaultVtxColor));

			// UV
			assert(texcoord_index < state->VertexTexcoordTempstorage.NumElements);
			memcpy(vtx.uv, state->VertexTexcoordTempstorage.Data[texcoord_index].uv, sizeof(vtx.uv));

			// 法线
			if (nor_index >= 0)
			{
				assert(nor_index < state->VertexNormalTempstorage.NumElements);
				memcpy(vtx.normal, state->VertexNormalTempstorage.Data[nor_index].position, sizeof(vtx.normal));
			}
			else
			{
				float defaultNormal[3] = {0.0f, 0.0f, 1.0f};
				memcpy(vtx.normal, defaultNormal, sizeof(defaultNormal));
			}

			state->Owner->VerticesCPU.NumElements = max(state->Owner->VerticesCPU.NumElements, state->NumOutputVertices);
			assert(state->Owner->VerticesCPU.NumElements * sizeof(decltype(*state->Owner->VerticesCPU.Data)) <= state->Owner->VerticesCPU.AllocBytes);
		}

		// 写入索引缓冲（用去重后的输出索引）
		state->Owner->IndicesCPU.GrowIfNeeded(MeshLoadingBufferGrowStep);
		state->Owner->IndicesCPU.Data[(state->NumIndicesLoaded)++] = outputIndex;
		state->Owner->IndicesCPU.NumElements = max(state->Owner->IndicesCPU.NumElements, state->NumIndicesLoaded);
		assert(state->Owner->IndicesCPU.NumElements * sizeof(decltype(*state->Owner->IndicesCPU.Data)) <= state->Owner->IndicesCPU.AllocBytes);
	}
}

bool RenderMeshResource::LoadMesh(std::istream& inputStream)
{
	tinyobj::callback_t cb;
	cb.vertex_cb = LoadVertexPositionCallback;
	cb.normal_cb = LoadNormalCallback;
	cb.texcoord_cb = LoadTexCoordCallback;
	cb.index_cb = LoadIndexCallback;
	//cb.usemtl_cb = usemtl_cb;
	//cb.mtllib_cb = mtllib_cb;
	//cb.group_cb = group_cb;
	//cb.object_cb = object_cb;

	VerticesCPU.Reset();
	IndicesCPU.Reset();

	MeshLoadingState loadingState(this);
	
	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObjWithCallback(inputStream, cb, &loadingState, nullptr, &warn, &err);

	if (!warn.empty())
	{
		printf("Warning when load mesh: %s\n", warn.c_str());
	}

	if (!err.empty())
	{
		printf("Error when load mesh: %s\n", err.c_str());
		return false;
	}

	assert(loadingState.NumOutputVertices > 0);
	assert(loadingState.NumIndicesLoaded > 0);
	assert(loadingState.NumIndicesLoaded % 3 == 0);

	return ret;
}

bool RenderMeshResource::LoadMesh_Plane(float quadWidth)
{
	static const RenderMeshResource::MeshVertex kVertexData[] =
	{
		// 3    4
		// +----+
		// |\   |
		// | \  |
		// |  \ |
		// |   \|
		// +----+
		// 1    2

		//   POSITION                COLOR               UV 				Normal
		{{-quadWidth/2, -quadWidth/2, 0.0f}, {0.7f, 0.7f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{quadWidth/2,  -quadWidth/2, 0.0f}, {0.7f, 0.7f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-quadWidth/2,  quadWidth/2, 0.0f}, {0.7f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{quadWidth/2,   quadWidth/2, 0.0f}, {1.0f, 0.7f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	};

	// CCW_FRONT
	// https://stackoverflow.com/questions/4346556/loading-indices-from-wavefront-obj-format
	static const tinyobj::index_t kIndexData[] =
	{
		{1, 0, 1},
		{2, 0, 2},
		{3, 0, 3},

		{2, 0, 2},
		{4, 0, 4},
		{3, 0, 3},
	};

	VerticesCPU.Reset();
	IndicesCPU.Reset();

	MeshLoadingState loadingState(this);

	for (int v = 0; v < (int)(sizeof(kVertexData) / sizeof(kVertexData[0])); ++v)
	{
		LoadVertexPositionCallback(&loadingState, kVertexData[v].position[0], kVertexData[v].position[1], kVertexData[v].position[2], 1.0f);
		LoadTexCoordCallback(&loadingState, kVertexData[v].uv[0], kVertexData[v].uv[1], 1.0f);
	}
	for (int i = 0; i < (int)(sizeof(kIndexData) / sizeof(kIndexData[0])); i += 3)
	{
		LoadIndexCallback(&loadingState, const_cast<tinyobj::index_t*>(&(kIndexData[i])), 3);
	}

	CullMode = CCW_FRONT;
	return true;
}


bool RenderMeshResource::LoadMesh_Sphere(float radius, int xdiv, int ydiv, float tx, float ty, float tz)
{
	VerticesCPU.Reset();
	IndicesCPU.Reset();

	VerticesCPU.NumElements = (xdiv + 1) * (ydiv + 1);
	VerticesCPU.Resize(VerticesCPU.NumElements);
	IndicesCPU.NumElements = (xdiv + 1) * (xdiv * (ydiv - 1) * 2) * 3;
	IndicesCPU.Resize(IndicesCPU.NumElements);

	memset(VerticesCPU.Data, 0, VerticesCPU.AllocBytes);
	memset(IndicesCPU.Data, 0, IndicesCPU.AllocBytes);

	// Everything else is just filling in the vertex and index buffer.
	MeshVertex* outV = VerticesCPU.Data;
	int* outI = IndicesCPU.Data;

	const float pi = 3.14159265358979323846f;
	const float gx = 2 * pi / xdiv;
	const float gy = pi / ydiv;

	for (long i = 0; i < xdiv; ++i)
	{
		const float theta = (float)i * gx;
		const float ct = cosf(theta);
		const float st = sinf(theta);

		const long k = i * (ydiv + 1);
		for (long j = 1; j < ydiv; ++j)
		{
			const float phi = (float)j * gy;
			const float sp = sinf(phi);
			const float x = ct * sp;
			const float y = st * sp;
			const float z = cosf(phi);

			float _pos[3] = { x * radius + tx , y * radius + ty , z * radius + tz };
			float _uv[2] = { theta * 0.1591549430918953f, phi * 0.31830988618379f };
			memcpy(outV[k + j].position, _pos, sizeof(_pos));
			// normal = Vector3(x, y, z);
			memcpy(outV[k + j].vtxcolor, DefaultVtxColor, sizeof(DefaultVtxColor));
			memcpy(outV[k + j].uv, _uv, sizeof(_uv));
		}
	}

	const long kk = xdiv * (ydiv + 1);
	for (long j = 1; j < ydiv; ++j)
	{
		const float phi = (float)j * gy;
		const float x = sinf(phi);
		const float z = cosf(phi);

		float _pos[3] = { x * radius + tx, ty, z * radius + tz };
		float _uv[2] = { 1.0f, phi * 0.31830988618379f };
		memcpy(outV[kk + j].position, _pos, sizeof(_pos));
		// normal = Vector3(x, 0, z);
		memcpy(outV[kk + j].vtxcolor, DefaultVtxColor, sizeof(DefaultVtxColor));
		memcpy(outV[kk + j].uv, _uv, sizeof(_uv));
	}

	for (long i = 0; i < xdiv; i++)
	{
		const long k1 = i * (ydiv + 1) + 1;
		const long k2 = (i + 1) * (ydiv + 1) + 1;
		const float s = (outV[k1].uv[0] + outV[k2].uv[0]) * 0.5f;

		float _pos[3] = { tx, ty, radius + tz };
		float _uv[2] = { s, 0 };
		memcpy(outV[k1 - 1].position, _pos, sizeof(_pos));
		// normal = Vector3(0, 0, 1);
		memcpy(outV[k1 - 1].vtxcolor, DefaultVtxColor, sizeof(DefaultVtxColor));
		memcpy(outV[k1 - 1].uv, _uv, sizeof(_uv));

		float _pos2[3] = { tx, ty, -radius + tz };
		float _uv2[2] = { s, 1 };
		memcpy(outV[k1 + ydiv - 1].position, _pos2, sizeof(_pos2));
		// normal = Vector3(0, 0, -1);
		memcpy(outV[k1 + ydiv - 1].vtxcolor, DefaultVtxColor, sizeof(DefaultVtxColor));
		memcpy(outV[k1 + ydiv - 1].uv, _uv2, sizeof(_uv2));
	}

	memcpy(outV[xdiv * (ydiv + 1)].position, outV[0].position, sizeof(float)*3);
	memcpy(outV[xdiv * (ydiv + 1)].vtxcolor, outV[0].vtxcolor, sizeof(DefaultVtxColor));
	memcpy(outV[xdiv * (ydiv + 1)].uv, outV[0].uv, sizeof(float)*2);

	memcpy(outV[xdiv * (ydiv + 1) + ydiv].position, outV[ydiv].position, sizeof(float) * 3);
	memcpy(outV[xdiv * (ydiv + 1) + ydiv].vtxcolor, outV[ydiv].vtxcolor, sizeof(DefaultVtxColor));
	memcpy(outV[xdiv * (ydiv + 1) + ydiv].uv, outV[ydiv].uv, sizeof(float) * 2);

	long ii = 0;
	for (long i = 0; i < xdiv; ++i)
	{
		const long k = i * (ydiv + 1);
		outI[ii + 0] = (int)k;
		outI[ii + 1] = (int)(k + 1);
		outI[ii + 2] = (int)(k + ydiv + 2);
		ii += 3;

		for (long j = 1; j < ydiv - 1; ++j)
		{
			outI[ii + 0] = (int)(k + j);
			outI[ii + 1] = (int)(k + j + 1);
			outI[ii + 2] = (int)(k + j + ydiv + 2);
			outI[ii + 3] = (int)(k + j);
			outI[ii + 4] = (int)(k + j + ydiv + 2);
			outI[ii + 5] = (int)(k + j + ydiv + 1);
			ii += 6;
		}

		outI[ii + 0] = (int)(k + ydiv - 1);
		outI[ii + 1] = (int)(k + ydiv);
		outI[ii + 2] = (int)(k + ydiv * 2);
		ii += 3;
	}

	// Double texcoords
	for (int i = 0; i < VerticesCPU.NumElements; ++i)
	{
		outV[i].uv[0] *= 4;
		outV[i].uv[1] *= 2;
	}

	CullMode = CCW_FRONT;
	return true;
}



bool RenderMeshResource::LoadMesh_Box(float width, float height, float depth)
{
	VerticesCPU.Reset();
	IndicesCPU.Reset();
	float halfW = width * 0.5f;
	float halfH = height * 0.5f;
	float halfD = depth * 0.5f;
	VerticesCPU.Resize(24);
	IndicesCPU.NumElements = 36;
	IndicesCPU.Resize(IndicesCPU.NumElements);
	MeshVertex* outV = VerticesCPU.Data;
	int* outI = IndicesCPU.Data;
	// 前面 normal = (0, 0, +1)
	outV[0]  = { {-halfW, -halfH,  halfD}, {1,1,1}, {0.0f, 0.0f}, {0, 0, 1} };
	outV[1]  = { { halfW, -halfH,  halfD}, {1,1,1}, {1.0f, 0.0f}, {0, 0, 1} };
	outV[2]  = { { halfW,  halfH,  halfD}, {1,1,1}, {1.0f, 1.0f}, {0, 0, 1} };
	outV[3]  = { {-halfW,  halfH,  halfD}, {1,1,1}, {0.0f, 1.0f}, {0, 0, 1} };

	// 后面 normal = (0, 0, -1)
	outV[4]  = { { halfW, -halfH, -halfD}, {1,1,1}, {0.0f, 0.0f}, {0, 0,-1} };
	outV[5]  = { {-halfW, -halfH, -halfD}, {1,1,1}, {1.0f, 0.0f}, {0, 0,-1} };
	outV[6]  = { {-halfW,  halfH, -halfD}, {1,1,1}, {1.0f, 1.0f}, {0, 0,-1} };
	outV[7]  = { { halfW,  halfH, -halfD}, {1,1,1}, {0.0f, 1.0f}, {0, 0,-1} };

	// 左面 normal = (-1, 0, 0)
	outV[8]  = { {-halfW, -halfH, -halfD}, {1,1,1}, {0.0f, 0.0f}, {-1, 0, 0} };
	outV[9]  = { {-halfW, -halfH,  halfD}, {1,1,1}, {1.0f, 0.0f}, {-1, 0, 0} };
	outV[10] = { {-halfW,  halfH,  halfD}, {1,1,1}, {1.0f, 1.0f}, {-1, 0, 0} };
	outV[11] = { {-halfW,  halfH, -halfD}, {1,1,1}, {0.0f, 1.0f}, {-1, 0, 0} };

	// 右面 normal = (+1, 0, 0)
	outV[12] = { { halfW, -halfH,  halfD}, {1,1,1}, {0.0f, 0.0f}, {1, 0, 0} };
	outV[13] = { { halfW, -halfH, -halfD}, {1,1,1}, {1.0f, 0.0f}, {1, 0, 0} };
	outV[14] = { { halfW,  halfH, -halfD}, {1,1,1}, {1.0f, 1.0f}, {1, 0, 0} };
	outV[15] = { { halfW,  halfH,  halfD}, {1,1,1}, {0.0f, 1.0f}, {1, 0, 0} };

	// 上面 normal = (0, +1, 0)
	outV[16] = { {-halfW,  halfH,  halfD}, {1,1,1}, {0.0f, 0.0f}, {0, 1, 0} };
	outV[17] = { { halfW,  halfH,  halfD}, {1,1,1}, {1.0f, 0.0f}, {0, 1, 0} };
	outV[18] = { { halfW,  halfH, -halfD}, {1,1,1}, {1.0f, 1.0f}, {0, 1, 0} };
	outV[19] = { {-halfW,  halfH, -halfD}, {1,1,1}, {0.0f, 1.0f}, {0, 1, 0} };

	// 下面 normal = (0, -1, 0)
	outV[20] = { {-halfW, -halfH, -halfD}, {1,1,1}, {0.0f, 0.0f}, {0,-1, 0} };
	outV[21] = { { halfW, -halfH, -halfD}, {1,1,1}, {1.0f, 0.0f}, {0,-1, 0} };
	outV[22] = { { halfW, -halfH,  halfD}, {1,1,1}, {1.0f, 1.0f}, {0,-1, 0} };
	outV[23] = { {-halfW, -halfH,  halfD}, {1,1,1}, {0.0f, 1.0f}, {0,-1, 0} };
	//索引
	int indices[] = {
		0,1,2, 0,2,3,		//前面
		4,5,6, 4,6,7,		//后面
		8,9,10, 8,10,11,	//左面
		12,13,14, 12,14,15,//右面
		16,17,18, 16,18,19,//上面
		20,21,22, 20,22,23 //下面
	};
	memcpy(outI, indices, sizeof(indices));
	CullMode = CCW_FRONT;
	return true;
}





bool RenderMeshResource::LoadMesh_Cylinder(float radius, float height, int segments)
{
	VerticesCPU.Reset();
	IndicesCPU.Reset();
	float halfH = height / 2.0f;
	const float pi = 3.14159265358979323846f;
	//侧面顶点+上下底面中心
	VerticesCPU.Resize((segments + 1) * 2 + 2);
	IndicesCPU.Resize(segments * 12);
	MeshVertex* outV = VerticesCPU.Data;
	int* outI = IndicesCPU.Data;
	int vertexIndex = 0;
	for (int i = 0; i <= segments; ++i)
	{
	    float angle = i * 2.0f * pi / segments;
	    float x  = cosf(angle) * radius;
	    float z  = sinf(angle) * radius;
	    float nx = cosf(angle);   // 径向法线 x 分量
	    float nz = sinf(angle);   // 径向法线 z 分量
	
	    // 侧面：法线朝外（nx, 0, nz）
	    outV[vertexIndex++] = { {x, -halfH, z}, {1,1,1}, {(float)i/segments, 0.0f}, {nx, 0, nz} };
	    outV[vertexIndex++] = { {x,  halfH, z}, {1,1,1}, {(float)i/segments, 1.0f}, {nx, 0, nz} };
	}
	
	// 顶面中心：法线朝上 (0, +1, 0)  —— 无需 nx/nz
	int topCenter = vertexIndex;
	outV[vertexIndex++] = { {0.0f,  halfH, 0.0f}, {1,1,1}, {0.5f, 0.5f}, {0, 1, 0} };
	
	// 底面中心：法线朝下 (0, -1, 0)  —— 无需 nx/nz
	int bottomCenter = vertexIndex;
	outV[vertexIndex++] = { {0.0f, -halfH, 0.0f}, {1,1,1}, {0.5f, 0.5f}, {0,-1, 0} };
	//索引
	int indexIndex = 0;
	//侧面
	for (int i = 0; i < segments; ++i)
	{
		int a = i * 2;
		int b = a + 1;
		int c = a + 2;
		int d = a + 3;
		outI[indexIndex++] = a;
		outI[indexIndex++] = b;
		outI[indexIndex++] = c;
		outI[indexIndex++] = b;
		outI[indexIndex++] = d;
		outI[indexIndex++] = c;
	}
	//顶面
	for (int i = 0; i < segments; ++i)
	{
		int a = i * 2 + 1;
		int b = (i + 1) * 2 + 1;
		outI[indexIndex++] = topCenter;
		outI[indexIndex++] = a;
		outI[indexIndex++] = b;
	}
	//底面
	for (int i = 0; i < segments; ++i) {
		int a = i * 2;
		int b = (i + 1) * 2;
		outI[indexIndex++] = bottomCenter;
		outI[indexIndex++] = b;
		outI[indexIndex++] = a;
	}
	CullMode = CCW_FRONT;
	return true;

}