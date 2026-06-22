//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "DebugVisCommon.hlsli"

RWByteAddressBuffer InCoords : register(u0);
RWTexture2D<uint>   OutKeys  : register(u1);

[RootSignature(DebugVisRootSig)]
[numthreads(64, 1, 1)]
void main(uint Id : SV_DispatchThreadID)
{
    if (Id >= dispatchSize)
        return;

    const uint2 Coords = UnpackU16FromU32(InCoords.Load((dispatch1dOffset + Id) << 2));

    uint Ret = 0;
    InterlockedCompareExchange(OutKeys[Coords], DEBUG_VIS_VERIFY_PIXEL_TOUCHED_INIT_KEY, DEBUG_VIS_VERIFY_PIXEL_TOUCHED_SEED_KEY, Ret);

    if (Ret == DEBUG_VIS_VERIFY_PIXEL_TOUCHED_SEED_KEY)
    {
        InterlockedCompareExchange(OutKeys[Coords], DEBUG_VIS_VERIFY_PIXEL_TOUCHED_SEED_KEY, DEBUG_VIS_VERIFY_PIXEL_TOUCHED_FAIL_KEY, Ret);
    }
}
