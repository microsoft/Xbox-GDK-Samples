//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

// Wave64 seems to defeat coissue, because it runs the instructions twice back-to-back.
// So the 4-instruction sequence in the loop runs in 11 clocks instead of 8 clocks.
#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#endif

RWStructuredBuffer<float3> buf : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main( uint3 id : SV_GroupThreadID, uint3 groupId : SV_GroupId )
{
    float x = asfloat(id.x);
    float y = asfloat(id.y);
    float z = asfloat(id.z);
    float x1 = asfloat(groupId.x);
    float y1 = asfloat(groupId.y);
    float z1 = asfloat(groupId.z);

    [unroll]
    for (uint i = 0; i < 128; ++i)
    {
        x = sqrt(x);
        x1 = x1 * x1;
        y1 = y1 * y1;
        z1 = z1 * z1;
    }

#pragma warning(disable:3583)
    buf[0] = float3(x,y,z);
    buf[1] = float3(x1,y1,z1);
}
