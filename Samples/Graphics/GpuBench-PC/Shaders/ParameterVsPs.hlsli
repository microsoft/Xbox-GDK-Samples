//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef NUM_EXPORTS
#error NUM_EXPORTS must be defined!
#endif

struct Interpolants
{
    float4 position : SV_Position;

    #if NUM_EXPORTS > 0
    float4 payload[NUM_EXPORTS]     : Payload;
    #endif
};

