//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


struct VS_INPUT
{
	uint vertexId : SV_VertexID;
};


struct VS_OUTPUT
{
	float4 pos      : SV_POSITION;
    float2 uv       : UV0;
};


[RootSignature(GlobalRS)]
VS_OUTPUT FullScreenTriangleVS(VS_INPUT input)
{    /*
        VertexId    Coord       UV
        0           -1,-1       0, 0
        1            3,-1       2, 0
        2           -1, 3       0, 2
    */
    VS_OUTPUT output;

    output.pos.x    = (input.vertexId == 1) ? 3.0 : -1.0;
    output.pos.y    = (input.vertexId == 2) ? 3.0 : -1.0;
    output.pos.zw   = float2(1.0, 1.0);
    output.uv       = output.pos.xy * 0.5 + 0.5;
    output.uv.y     = 1.0 - output.uv.y;

  	return output;
}
