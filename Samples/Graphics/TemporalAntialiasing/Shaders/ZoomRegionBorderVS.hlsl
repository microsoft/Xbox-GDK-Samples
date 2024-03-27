//--------------------------------------------------------------------------------------
// ZoomRegionBorderVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignatures.hlsli"

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

ConstantBuffer<ZoomPassCB> cts : register(b0);

[RootSignature(ZoomBorderPassRS)]
float4 main(uint vI : SV_VertexId) : SV_POSITION
{
    float4 position = 0.0f;

    float2 minNDC = cts.minUV * 2.0f - 1.0f;
    float2 maxNDC = cts.maxUV * 2.0f - 1.0f;

    switch (vI)
    {
    case(0): // Bottom - Left
        position = float4(minNDC.x, minNDC.y, 0.0f, 1.0f);
        break;
    case(1): // Top - Left
        position = float4(minNDC.x, maxNDC.y, 0.0f, 1.0f);
        break;
    case(2): // Bottom - Right
        position = float4(maxNDC.x, minNDC.y, 0.0f, 1.0f);
        break;
    case(3): // Top - Right
        position = float4(maxNDC.x, maxNDC.y, 0.0f, 1.0f);
        break;
    case(4): // Bottom - Right
        position = float4(maxNDC.x, minNDC.y, 0.0f, 1.0f);
        break;
    case(5): // Top - Left
        position = float4(minNDC.x, maxNDC.y, 0.0f, 1.0f);
        break;
    }

    return position;
}
