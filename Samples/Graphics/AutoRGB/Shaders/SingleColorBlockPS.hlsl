//--------------------------------------------------------------------------------------
// RenderHistogramPS.hlsl
//
// A simple pixel shader to render a texture 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

cbuffer PSHData : register(b0)
{
    float r, g, b, a;
};

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    return float4(r, g, b, a);
}
