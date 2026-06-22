//--------------------------------------------------------------------------------------
// File: GetStartTime.hlsl
//
// This is a compute shader that stores the current time from the GPU clock, to be used
// in simulating GPU load. It is not part of the dynamic resolution implementation.
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
        "UAV(u0)"\
    )\
]

RWStructuredBuffer<Time> TimeBuffer : register(u0);

[numthreads(1, 1, 1)]
RS
void main(uint3 DTid : SV_DispatchThreadID)
{
    TimeBuffer[0].startTime = GetTimeStamp();
}
