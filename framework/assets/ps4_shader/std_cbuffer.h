/* SIE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 09.008.001
* Copyright (C) 2013 Sony Interactive Entertainment Inc.
*/

#ifndef __STDCBUFFER_H__
#define __STDCBUFFER_H__

#include "framework/render/PS4/api_gnm/toolkit/shader_common/shader_base.h"


unistruct ShaderConstants
{
	Matrix4Unaligned m_WorldViewProj;
};

#define TranformPoint(tranMat, posVec) mul(posVec, tranMat)
#endif
