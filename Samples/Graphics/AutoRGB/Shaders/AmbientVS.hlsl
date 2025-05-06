//--------------------------------------------------------------------------------------
// AmbientVS.hlsl
//
// Vertex Shader for the Ambient Pass (Fullscreen pass to render dim albedo).
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../shared.h"
#include "GPassCommon.hlsli"

struct Interpolators
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

[RootSignature(ROOT_SIGNATURE_AMBIENT)]
Interpolators main(in uint vI : SV_VertexID)
{
    Interpolators output;

    // We use the 'big triangle' optimization so you only Draw 3 verticies instead of 4.
    float2 texcoord = float2((vI << 1) & 2, vI & 2);
    output.TexCoord = texcoord;
    output.Position = float4(texcoord.x * 2.0f - 1.0f, -texcoord.y * 2.0f + 1.0f, 0.0f, 1.0f);

    return output;
}
