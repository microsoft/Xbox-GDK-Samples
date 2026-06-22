//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

[ROOT_SIGNATURE]
void main(float4 position : SV_Position, out uint stencilRef : SV_StencilRef)
{
    // This needs to vary per pixel of each tile or stencil will single-value compress.
    uint2 microPosition = ((uint2) (position.xy - 0.5f)) & 0x0f;
    stencilRef = microPosition.x | (microPosition.y << 4);
}
