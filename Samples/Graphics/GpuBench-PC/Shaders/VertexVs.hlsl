//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_PS_NGG 
#define __XBOX_PRECOMPILE_VS_PS_LGG
#endif

struct Vertex
{
};

struct Interpolants
{
    float4 position     : SV_Position;
};

[ROOT_SIGNATURE]
Interpolants main( Vertex In )
{
    Interpolants Out = (Interpolants)0;

    // Primitives with all vertices at this location will get frustum-culled
    // (or zero-area culled in the case of RECTLIST)
    Out.position = float4(2.0f, 0.0f, 0.0f, 1.0f);

    return Out;
}
