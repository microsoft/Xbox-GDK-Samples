//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifdef PREFER_LATE_Z
StructuredBuffer<uint> dummyBuffer : register(t0);
#endif

[ROOT_SIGNATURE]
float4 main() : SV_Target
{
#ifdef PREFER_LATE_Z
    /** dummy condition with discard on PC to enable LateZ */
    if (dummyBuffer[0] == 123)
    {
        discard;
    }
#endif
    return 0.0f;
}
