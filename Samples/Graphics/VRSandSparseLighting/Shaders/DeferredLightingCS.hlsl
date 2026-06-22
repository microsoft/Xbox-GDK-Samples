//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32                // very important, don't forget this!

#define SPARSE_LIGHTING                     0

#include "LightingShader.hlsli"



// although common resolutions (2160p, 1440p) do divide into exactly 16 (lighting tile size)
// better to use a minimal group size for DRS
[RootSignature(GlobalRS)]
[numthreads(8, 4, 1)]
void DeferredLightingCS(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID)
{
    DoLighting(Gid, GTid);
}
