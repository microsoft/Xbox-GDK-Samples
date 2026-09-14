//--------------------------------------------------------------------------------------
// File: Time.hlsli
//
// Helpers for reading the real-time GPU clock used by the artificial workloads.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct Time
{
    uint64_t startTime;
    uint64_t currentTime;
};

uint64_t GetTimeStamp()
{
    uint2 memtime = __XB_MemTime();
    return ((uint64_t)memtime.y << 32) | memtime.x;
}
