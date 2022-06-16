//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

struct DebugVertex
{
    float4 Color      : COLOR0;
    float4 Position   : SV_POSITION;
};

// Rotates a vector, v0, about an axis by some angle
float3 RotateVector(float3 v0, float3 axis, float angle)
{
    float cs = cos(angle);
    return cs * v0 + sin(angle) * cross(axis, v0) + (1 - cs) * dot(axis, v0) * axis;
}

bool IsConeDegenerate(CullData c)
{
    return (c.NormalCone >> 24) == 0xff;
}

float4 UnpackCone(uint packed)
{
    float4 v;
    v.x = float((packed >> 0) & 0xFF);
    v.y = float((packed >> 8) & 0xFF);
    v.z = float((packed >> 16) & 0xFF);
    v.w = float((packed >> 24) & 0xFF);

    v = v / 255.0;
    v.xyz = v.xyz * 2.0 - 1.0;

    return v;
}

uint PrefixCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT
    return __XB_MBCNT64(__XB_Ballot64(flag));
#else
    return WavePrefixCountBits(flag);
#endif
}

uint ActiveCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT
    return __XB_S_BCNT1_U64(__XB_Ballot64((uint)flag));
#else
    return WaveActiveCountBits(flag);
#endif
}
