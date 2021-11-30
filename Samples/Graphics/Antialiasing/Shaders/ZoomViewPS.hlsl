//--------------------------------------------------------------------------------------
// ZoomViewPS.hlsl
//
// Pixel shader for zoomed view
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    uint2 viDims;
    Texture.GetDimensions(viDims.x, viDims.y);
    return Texture.Load(int3(floor(In.TexCoord.xy * viDims), 0));
}
