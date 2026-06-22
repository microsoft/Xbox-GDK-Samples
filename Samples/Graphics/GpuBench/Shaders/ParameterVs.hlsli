//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef NUM_EXPORTS
#error NUM_EXPORTS must be defined!
#endif

// Seems to be a little slower
// #ifdef __XBOX_SCARLETT
// #define __XBOX_ENABLE_WAVE32 1
// #endif

struct Vertex
{
};

struct Interpolants
{
    float4 position : SV_Position;

#if NUM_EXPORTS > 0
    float4 payload[NUM_EXPORTS]     : Payload;
#endif
};

[ROOT_SIGNATURE]
Interpolants main(Vertex In)
{
    Interpolants Out = (Interpolants) 0;

    // Primitives with all vertices at this location will get frustum-culled
    // (or zero-area culled in the case of RECTLIST)
    Out.position = float4(2.0f, 0.0f, 0.0f, 1.0f);

    return Out;
}
