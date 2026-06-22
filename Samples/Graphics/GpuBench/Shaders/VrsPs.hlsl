//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target
{
    float4 color = In.texcoord.xyxy;

    for(uint i = 0; i < 512; ++i)
    {
        color.x *= color.x;
    }

    return color;
}
