//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


struct VS_INPUT
{
    uint vertexId   : SV_VertexID;
    uint instanceID : SV_InstanceID;
};


struct VS_OUTPUT
{
    float4 pos      : SV_POSITION;
    float3 worldPos	: UV0;
#if DEBUG_LODS == 1
    uint debug      : UV2;
#endif
};


[RootSignature(GlobalRS)]
VS_OUTPUT TerrainRenderVS(VS_INPUT input)
{
    VS_OUTPUT output;

    uint tileID = InstanceBufferSRV[rootConstantCB1][input.instanceID];
    int terrainTileSize = 1u << terrainTileSizeLog2;
    uint tileX = tileID & 0xffff;
    uint tileY = tileID >> 16;

    // workaround for machines that don't have an integer mod/div unit, saving over 120 bytes of instructions
    int2 location;
    location.y = int(floor(float(input.vertexId) / float(terrainTileSize + 1)));
    location.x = input.vertexId - (location.y * (terrainTileSize + 1));
    location += int2(tileX << terrainTileSizeLog2, tileY << terrainTileSizeLog2);

    float height = HeightMapSRV[location].x;

    if (0.0 == height)
    {
        // reading outside of texture 
        float nan       = asfloat(0x7fc00000);
        output.pos      = float4(nan, nan, nan, nan);
    }
    else
    {
        float3 pos      = float3(location.x, height, location.y);
        float3 wPos     = float3(pos.x * worldScale + xzTranslation, height, pos.z * worldScale + xzTranslation);
        output.pos      = mul(float4(wPos, 1.0f), wvpMatrix);
        output.worldPos = wPos;
    }
#if DEBUG_LODS == 1
    output.debug = rootConstantCB1;
#endif
    return output;
}
