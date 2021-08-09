//--------------------------------------------------------------------------------------
// SkyPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(SceneRS)]
float4 main(SceneInterpolants In) : SV_TARGET
{
    return texDiffuse.Sample(sampLinear, In.UV);
}
