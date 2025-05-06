//--------------------------------------------------------------------------------------
// RampCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define RS \
[\
    RootSignature\
    (\
       "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"\
    )\
]

RWTexture2D<float4> OutputTexture : register(u0);

cbuffer Constants : register(b0)
{
    float ColorScale;
};

[numthreads(8, 8, 1)]
RS
void main(uint3 id : SV_DispatchThreadID)
{
    const int numBands = 7;
    const int bandHeight = 2160 / numBands;

    // Write the id.x as a ramp value
    if (id.y < bandHeight)
    {
        OutputTexture[id.xy] = float4(id.x, 0.0f, 0.0f, 1.0f);
    }
    else if (id.y < bandHeight * 2)
    {
        OutputTexture[id.xy] = float4(0.0f, id.x, 0.0f, 1.0f);
    }
    else if (id.y < bandHeight * 3)
    {
        OutputTexture[id.xy] = float4(0.0f, 0.0f, id.x, 1.0f);
    }
    else if (id.y < bandHeight * 4)
    {
        OutputTexture[id.xy] = float4(id.x, id.x, 0.0f, 1.0f);
    }
    else if (id.y < bandHeight * 5)
    {
        OutputTexture[id.xy] = float4(0.0f, id.x, id.x, 1.0f);
    }
    else if (id.y < bandHeight * 6)
    {
        OutputTexture[id.xy] = float4(id.x, 0.0f, id.x, 1.0f);
    }
    else
    {
        OutputTexture[id.xy] = float4(id.x, id.x, id.x, 1.0f);
    }
}
