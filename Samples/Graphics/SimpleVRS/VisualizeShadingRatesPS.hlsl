//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
// lifted from the spec https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
#define D3D12_SHADING_RATE_1X1 0x0      // No change to shading rate
#define D3D12_SHADING_RATE_1X2 0x1      // Reduces vertical resolution 2x
#define D3D12_SHADING_RATE_2X1 0x4      // Reduces horizontal resolution 2x
#define D3D12_SHADING_RATE_2X2 0x5      // Reduces both axes by 2x

#include "FullScreenQuad.hlsli"

Texture2D<uint> ShadingRateImage : register(t1);
cbuffer VRSCB : register(b0)
{
    float2  shadingRateImageDimensions;
};

[RootSignature(FullScreenQuadRS)]
float4 VisualizeShadingRatesPS(Interpolators In) : SV_Target
{
    uint shadingRate = ShadingRateImage[int2(In.TexCoord * shadingRateImageDimensions.xy)].x;

    float4 color;
    switch (shadingRate)
    {
    case D3D12_SHADING_RATE_1X1:    color = float4(1.0, 0.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_1X2:    color = float4(1.0, 1.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X1:    color = float4(0.0, 1.0, 1.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X2:    color = float4(0.0, 1.0, 0.0, 0.0);   break;
    default:                        color = float4(1.0, 0.0, 1.0, 1.0);   break;  // should never happen!
    }
    return 0.25f * color;
}
