//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE\
    RootSignature\
    (\
        "\
            SRV(t0, visibility=SHADER_VISIBILITY_GEOMETRY),\
            DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),\
            StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL),\
        "\
    )\

struct Empty 
{
};

struct Interpolants
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct Scale
{
    float2 xy;
};
StructuredBuffer<Scale> buf : register(t0);

[maxvertexcount(4)]
[ROOT_SIGNATURE]
void main(point Empty Points[1], inout TriangleStream<Interpolants> Stream, uint primitive : SV_PrimitiveID)
{
    Interpolants Output[] = 
    {
        { { -1.0f, -1.0f, 1.0f, 1.0f, }, { 0.0f, 1.0f, }, },
        { { -1.0f,  1.0f, 1.0f, 1.0f, }, { 0.0f, 0.0f, }, },
        { {  1.0f, -1.0f, 1.0f, 1.0f, }, { 1.0f, 1.0f, }, }, 
        { {  1.0f,  1.0f, 1.0f, 1.0f, }, { 1.0f, 0.0f, }, }, 
    };

    // Scale each point sprite by the factor in the scale buffer
    // If the factors are near 0.0f, then this draw call is reasonably sized
    // If the factors are near 1.0f, then this draw call has duration of multiple seconds
    for (uint i = 0; i < 4; ++i)
    {
        float2 scale = buf[primitive].xy;

        Output[i].position.xy *= scale;
    }

    Stream.Append( Output[0] );
    Stream.Append( Output[1] );
    Stream.Append( Output[2] );
    Stream.Append( Output[3] );

    Stream.RestartStrip();
}
