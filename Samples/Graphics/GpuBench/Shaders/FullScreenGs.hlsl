//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Empty {};

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

// Geometry shader which generates a full screen triangle (which gets clipped/culled to the viewport)
[maxvertexcount(3)]
[ROOT_SIGNATURE]
void main(point Empty Points[1], inout TriangleStream<InterpolantsTexcoord> Stream)
{
    InterpolantsTexcoord Output[] = 
    {
        { { -1.0f, -1.0f, 1.0f, 1.0f, }, {  0.0f, +1.0f, }, },
        { { -1.0f, +3.0f, 1.0f, 1.0f, }, {  0.0f, -1.0f, }, },
        { { +3.0f, -1.0f, 1.0f, 1.0f, }, { +2.0f, +1.0f, }, }, 
    };

    Stream.Append( Output[0] );
    Stream.Append( Output[1] );
    Stream.Append( Output[2] );
}
