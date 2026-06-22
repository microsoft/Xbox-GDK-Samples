//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32                         // Deblocker runs best with wave32
#define DEBLOCK_SHARPEN_FILTER  1

#include "Deblock.hlsli"


[RootSignature(GlobalRS)]
[numthreads(16, 16, 1)]
void DeblockAndTonemapCS(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint2 DTid : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex)
{
    float3 colour = Deblock1616(LitOutputSRV, LinearDepthSRV, Gid, GTid, DTid, groupThreadIndex);

    PostProcessUAV[DTid] = ToneMap(colour);
}
