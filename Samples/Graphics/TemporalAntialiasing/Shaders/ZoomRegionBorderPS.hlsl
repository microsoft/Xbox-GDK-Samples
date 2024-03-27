//--------------------------------------------------------------------------------------
// ZoomRegionBorderPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

#include "RootSignatures.hlsli"

ConstantBuffer<ZoomPassCB> cts : register(b0);

struct Interpolators
{
    float4 Position : SV_POSITION;
};

[RootSignature(ZoomBorderPassRS)]
float4 main(Interpolators IN) : SV_TARGET
{
    float minX = cts.minUV.x * cts.width;
    float maxX = cts.maxUV.x * cts.width;
    float minY = cts.minUV.y * cts.height;
    float maxY = cts.maxUV.y * cts.height;

    if (IN.Position.x > (minX + cts.margin) && IN.Position.x < (maxX - cts.margin) &&
        IN.Position.y > (minY + cts.margin) && IN.Position.y < (maxY - cts.margin) )
    {
        discard;
    }

    return float4(cts.borderColor, 1.0f);
}
