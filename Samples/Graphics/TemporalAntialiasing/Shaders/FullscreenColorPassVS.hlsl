//--------------------------------------------------------------------------------------
// FullscreenColorPassVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignatures.hlsli"

struct Interpolators
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORDS;
};

Interpolators main11(uint vI : SV_VertexId)
{
    Interpolators output;

    // We use the 'big triangle' optimization so you only Draw 3 verticies instead of 4.
    float2 texcoord = float2((vI << 1) & 2, vI & 2);
    output.TexCoord = texcoord;
    output.Position = float4(texcoord.x * 2.0f - 1.0f, texcoord.y * -2.0f + 1.0f, 0.0f, 1.0f);

    return output;
}

[RootSignature(FullscreenPassRS)]
Interpolators main(uint vI : SV_VertexId)
{
    return main11(vI);
}
