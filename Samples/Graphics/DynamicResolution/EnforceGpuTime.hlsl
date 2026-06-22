//--------------------------------------------------------------------------------------
// File: EnforceGpuTime.hlsl
//
// This is a compute shader that simulates load on the GPU by spinning until a specified
// time has passed. It is not part of the dynamic resolution implementation.
//
// This should be compiled with /Od, or subsequent calls to __XB_MemTime() (in
// GetTimeStamp()) may be optimized away.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Time.hlsli"

#define RS \
[\
    RootSignature\
    (\
       "RootConstants(num32BitConstants=2, b0),\
        UAV(u0)"\
    )\
]

cbuffer cb0
{
    uint64_t g_desiredFrameTime;
}

RWStructuredBuffer<Time> TimeBuffer : register(u0);

[numthreads(1, 1, 1)]
RS
void main( uint3 DTid : SV_DispatchThreadID )
{
    do
    {
        // Use a buffer to prevent these calls from being optimized away
        TimeBuffer[0].currentTime = GetTimeStamp();
    }
    while ((TimeBuffer[0].currentTime - TimeBuffer[0].startTime) < g_desiredFrameTime);
}
