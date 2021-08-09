//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Mesh.hlsli"

[ROOT_SIGNATURE_MESH]
float4 main(PSSceneIn input) : SV_Target
{
    return g_color * lerp(0.5f, 1.0f, dot(input.m_normal, g_lightDir));
}

