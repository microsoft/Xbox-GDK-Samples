//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<uint> buf : register(u0);

[numthreads(THREADGROUP_X, THREADGROUP_Y, THREADGROUP_Z)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_GroupThreadID)
{
    uint dummy = 0;

#if VGPR_LOAD >= 1
    dummy += id.x;
#endif
#if VGPR_LOAD >= 2
    dummy += id.y;
#endif
#if VGPR_LOAD >= 3
    dummy += id.z;
#endif

    if (dummy == 1000000)
    {
        buf[0] = 0;
    }
}
