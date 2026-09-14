//--------------------------------------------------------------------------------------
// File: GetStartTime.hlsl
//
// This compute shader stores the current GPU clock value used as the starting point for
// FramePacing's repeatable artificial GPU workloads.
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
