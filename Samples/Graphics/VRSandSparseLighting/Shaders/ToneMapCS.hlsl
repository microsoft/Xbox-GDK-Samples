//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32

#include "Global.hlsli"


[RootSignature(GlobalRS)]
[numthreads(8, 4, 1)]
void ToneMapCS(uint2 DTid : SV_DispatchThreadID)
{
    float3 colour;

    if(rootConstantCB1)
        colour = PostProcessUAV[DTid];
    else
        colour = LitOutputSRV[DTid];

    PostProcessUAV[DTid] = ToneMap(colour);
}
