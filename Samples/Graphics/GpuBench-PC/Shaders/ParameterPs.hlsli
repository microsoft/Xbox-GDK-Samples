//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParameterVsPs.hlsli"

[ROOT_SIGNATURE]
float4 main(Interpolants In) : SV_Target
{
    float4 accumulator = 0.0;
#if NUM_EXPORTS > 0
    for (uint i = 0; i < NUM_EXPORTS; ++i)
    {
        // make sure all interpolats are used to avoid any optimisation targeting unused parameter removal
        accumulator += In.payload[i];
    }
#endif
    return accumulator;
}
