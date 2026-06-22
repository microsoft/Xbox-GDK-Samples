//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

StructuredBuffer<uint4> buf : register(t0);
RWStructuredBuffer<uint4> rwBuf : register(u0);

[numthreads(64, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 id: SV_DispatchThreadID)
{
    rwBuf[id.x] = buf[id.x] + 1;
}
