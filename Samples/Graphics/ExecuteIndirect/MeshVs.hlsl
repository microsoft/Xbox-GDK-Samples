//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Mesh.hlsli"

#if defined(__XBOX_SCARLETT)
#define __XBOX_PRECOMPILE_VS_PS 0
#define __XBOX_PRECOMPILE_VS_GS 1
#endif

[ROOT_SIGNATURE_MESH]
GSSceneIn main(VSSceneIn input)
{
    GSSceneIn output;

    output.m_position = mul(float4(input.m_position, 1.0), g_worldViewProj);
    output.m_worldPos = mul(input.m_position, (float3x3)g_world);

    return output;
}

