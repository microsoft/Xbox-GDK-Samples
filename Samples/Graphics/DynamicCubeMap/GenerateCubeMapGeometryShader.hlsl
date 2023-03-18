//------------------------------------------------------------------------------------
// GenerateCubeMapGeometryShader.hlsl
//
// Shader to generate an environment map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GenerateCubeMap.hlsli"

[maxvertexcount(18)]
[RootSignature(MainRS)]
void main(
    triangle GS_CUBEMAP_IN input[3],
    inout TriangleStream< PS_CUBEMAP_IN > CubeMapStream)
{
    for (int cubeSide = 0; cubeSide < 6; ++cubeSide)
    {
        // Compute screen coordinates
        PS_CUBEMAP_IN output;
        output.RTIndex = cubeSide;
        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(input[v].Pos, cubeMapConstants.mViewCBM[cubeSide]);
            output.Pos = mul(output.Pos, constants.mProj);
            output.Tex = input[v].Tex;
            CubeMapStream.Append(output);
        }
        CubeMapStream.RestartStrip();
    }
}
