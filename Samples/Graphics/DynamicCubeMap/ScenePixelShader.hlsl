//------------------------------------------------------------------------------------
// ScenePixelShader.hlsl
//
// Simple shader to render a textured quad
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Scene.hlsli"

[RootSignature(MainRS)]
Pixel main(Interpolants In)
{
    Pixel Out;
    Out.color = txDiffuse.Sample(samLinear, In.texcoord);
    return Out;
}
