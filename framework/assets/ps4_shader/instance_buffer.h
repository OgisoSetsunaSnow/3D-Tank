#ifndef __INSTANCE_BUFFER_H__
#define __INSTANCE_BUFFER_H__

#include "framework/render/PS4/api_gnm/toolkit/shader_common/shader_base.h"

#ifdef __cplusplus
#define  REGISTER(x)
#else
#define  REGISTER(x) :register(x)
#endif //__cplusplus

struct PerInstanceData3D
{
	Matrix4Unaligned xform;
	Vector4Unaligned uv_offset_scale;
	Vector4Unaligned color;
};

struct PerInstanceData2D
{
	Matrix4Unaligned xform;	// only 3x3 is used
	Vector4Unaligned uv_offset_scale;
	Vector4Unaligned color;
};

#ifdef __PSSL__
RegularBuffer<PerInstanceData3D> instanceData3D REGISTER(t4);
RegularBuffer<PerInstanceData2D> instanceData2D REGISTER(t5);
#endif

#endif
