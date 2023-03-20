//------------------------------------------------------------------------------------
// GenerateCubeMapPixelShader.hlsl
//
// Shader to generate an environment map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GenerateCubeMap.hlsli"

struct Pixel
{
    float4 color    : SV_Target;
};

[RootSignature(MainRS)]
Pixel main(PS_CUBEMAP_IN input)
{
    Pixel Out;
    Out.color = txDiffuse.Sample(samLinear, input.Tex);
    return Out;
}
