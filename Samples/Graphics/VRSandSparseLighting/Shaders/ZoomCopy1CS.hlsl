//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define __XBOX_ENABLE_WAVE32

#include "ZoomCopy.hlsli"


[RootSignature(GlobalRS)]
[numthreads(8, 4, 1)]
void ZoomCopyCS(uint2 DTid : SV_DispatchThreadID)
{
    ZoomCopy(DTid, BackBuffer1UAV);
}
