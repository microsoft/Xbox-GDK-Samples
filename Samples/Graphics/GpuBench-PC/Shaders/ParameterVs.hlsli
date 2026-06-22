//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParameterVsPs.hlsli"

// Seems to be a little slower
// #ifdef __XBOX_SCARLETT
// #define __XBOX_ENABLE_WAVE32 1
// #endif

struct Vertex
{
};

[ROOT_SIGNATURE]
Interpolants main(Vertex In)
{
#if defined(__XBOX_SCARLETT) || defined(__XBOX_ONE)
    Interpolants Out = (Interpolants) 0;

    // Primitives with all vertices at this location will get frustum-culled
    // (or zero-area culled in the case of RECTLIST)
    Out.position = float4(2.0f, 0.0f, 0.0f, 1.0f);
#else
    Interpolants Out;

#if NUM_EXPORTS > 0
    for (uint i = 0; i < NUM_EXPORTS; ++i)
    {
        // make sure all interpolants aren't duplicate to avoid any optimisation targeting de-deduplication
        Out.payload[i] = float(i + 1) * float4(1.1, 1.11, 1.111, 1.111) ;
    }
#endif

    // Make sure that primitives are not frustum culled, so any potential optimisations related to attribute-exporting (on culled vertices are avoided)
    Out.position = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif

    return Out;
}
