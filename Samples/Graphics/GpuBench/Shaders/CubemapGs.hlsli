//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef FETCH_DIRECTION
#define FETCH_DIRECTION {1.0f, 0.0f, 0.0f, }
#endif

struct Empty {};

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float3 texcoord     : TEXCOORD0;
};

// Geometry shader which generates a full screen triangle (which gets clipped/scissored to the viewport)
[maxvertexcount(3)]
[ROOT_SIGNATURE]
void main(point Empty Points[1], inout TriangleStream<InterpolantsTexcoord> Stream)
{
    InterpolantsTexcoord Output[] = 
    {
        { { -1.0f, -1.0f, 0.0f, 1.0f, }, { FETCH_DIRECTION }, },
        { { -1.0f, +3.0f, 0.0f, 1.0f, }, { FETCH_DIRECTION }, },
        { { +3.0f, -1.0f, 0.0f, 1.0f, }, { FETCH_DIRECTION }, }, 
    };

    Stream.Append( Output[0] );
    Stream.Append( Output[1] );
    Stream.Append( Output[2] );
}
