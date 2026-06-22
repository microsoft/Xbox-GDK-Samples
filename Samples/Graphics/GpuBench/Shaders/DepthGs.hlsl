//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Empty {};

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
};

// Geometry shader which generates a full screen triangle (which gets clipped/culled to the viewport)
// Render at depth of 0.5f - 1.0f, increasing with primitive id, so it's easy to make everything pass or fail depth test
[maxvertexcount(3)]
[ROOT_SIGNATURE]
void main(point Empty Points[1], inout TriangleStream<InterpolantsTexcoord> Stream, uint prim : SV_PrimitiveID)
{
    InterpolantsTexcoord Output[] = 
    {
        { { -1.0f, -1.0f, 0.5f + prim / 1024.0f, 1.0f, }, },
        { { -1.0f, +3.0f, 0.5f + prim / 1024.0f, 1.0f, }, },
        { { +3.0f, -1.0f, 0.5f + prim / 1024.0f, 1.0f, }, },
    };

    Stream.Append( Output[0] );
    Stream.Append( Output[1] );
    Stream.Append( Output[2] );
}
