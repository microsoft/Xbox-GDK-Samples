//--------------------------------------------------------------------------------------
// SimpleTrianglePS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

[RootSignature("")]
float4 main(VertexOut input) : SV_TARGET
{
    return input.Color;
}
