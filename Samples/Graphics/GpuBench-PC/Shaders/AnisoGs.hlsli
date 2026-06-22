//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef ANISO_LEVEL
#define ANISO_LEVEL 1.0f
#endif

#include "RootSignature.hlsli"

struct Empty {};

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

// Geometry shader which generates a full screen triangle (which gets clipped/scissored to the viewport)
[maxvertexcount(3)]
[ROOT_SIGNATURE]
void main(point Empty Points[1], inout TriangleStream<InterpolantsTexcoord> Stream)
{
    // This triangle will hit the max aniso level up to 16, 
    // since one gradient is longer than the other in texture space.
    // The bias is to prevent rounding error (e.g. ANISO_LEVEL of exactly 6.0 otherwise results in 8 taps)
    float anisoSkew = max(1, ANISO_LEVEL * 0.999f);
    InterpolantsTexcoord Output[] = 
    {
        { { -1.0f, -1.0f, 0.0f, 1.0f, }, {  0.0f,                0.0f,   }, },
        { { -1.0f, +3.0f, 0.0f, 1.0f, }, {  0.0f,               +2.0f,   }, },
        { { +3.0f, -1.0f, 0.0f, 1.0f, }, { +2.0f * anisoSkew,    0.0f,   }, }, 
    };

    Stream.Append( Output[0] );
    Stream.Append( Output[1] );
    Stream.Append( Output[2] );
}
