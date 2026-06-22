//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

Texture2D<float4> tex       : register(t0);
RWTexture2D<float4> rwTex   : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main(uint3 id: SV_DispatchThreadID)
{
    rwTex[id.xy] = tex[id.xy];
}
