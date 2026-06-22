//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Geometry shader which generates a full screen triangle (which gets clipped/culled)
//-------------------------------------------------------------------------------------------------------------
[RootSignature(ROOT_SIG)]
[maxvertexcount(3)]
void main(point InterpolantsPoint Points[1], inout TriangleStream<InterpolantsTexcoord> Stream)
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
