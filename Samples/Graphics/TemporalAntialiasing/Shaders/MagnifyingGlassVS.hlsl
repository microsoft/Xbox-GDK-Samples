//--------------------------------------------------------------------------------------
// MagnifyingGlassVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignatures.hlsli"

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

struct Interpolators
{
    float4 Position : SV_POSITION;
    float2 texCoords : TEXCOORD;
};

ConstantBuffer<MagnifyingGlassPassCB> cts : register(b0);

[RootSignature(MagnifyingGlassPassRS)]
Interpolators main(uint vI : SV_VertexId)
{
    Interpolators output = (Interpolators)0;

    switch (vI)
    {
    case(0): // Bottom - Left
        output.Position = float4(cts.vertexMinCoordsNDC, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMinUV.x, cts.texMaxUV.y);
        break;
    case(1): // Top - Left
        output.Position = float4(cts.vertexMinCoordsNDC.x, cts.vertexMaxCoordsNDC.y, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMinUV.x, cts.texMinUV.y);
        break;
    case(2): // Bottom - Right
        output.Position = float4(cts.vertexMaxCoordsNDC.x, cts.vertexMinCoordsNDC.y, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMaxUV.x, cts.texMaxUV.y);
        break;
    case(3): // Top - Right
        output.Position = float4(cts.vertexMaxCoordsNDC, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMaxUV.x, cts.texMinUV.y);
        break;
    case(4): // Bottom - Right
        output.Position = float4(cts.vertexMaxCoordsNDC.x, cts.vertexMinCoordsNDC.y, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMaxUV.x, cts.texMaxUV.y);
        break;
    case(5): // Top - Left
        output.Position = float4(cts.vertexMinCoordsNDC.x, cts.vertexMaxCoordsNDC.y, 0.0f, 1.0f);
        output.texCoords = float2(cts.texMinUV.x, cts.texMinUV.y);
        break;
    }

    return output;
}
