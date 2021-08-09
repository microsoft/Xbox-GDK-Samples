//--------------------------------------------------------------------------------------
// Shared.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define CommonRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), \
     RootConstants(num32BitConstants=3, b0), \
     SRV(t0), \
     DescriptorTable(SRV(t1), visibility=SHADER_VISIBILITY_PIXEL), \
     StaticSampler(s0, \
            filter = FILTER_MIN_MAG_LINEAR_MIP_POINT, \
            addressU = TEXTURE_ADDRESS_BORDER, \
            addressV = TEXTURE_ADDRESS_BORDER, \
            addressW = TEXTURE_ADDRESS_BORDER, \
            maxAnisotropy = 1,\
            comparisonFunc = COMPARISON_NEVER, \
            borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, \
            visibility = SHADER_VISIBILITY_PIXEL)"

//------------------------------------
// Shared Resources

cbuffer b : register(b0)
{
    float2  invViewportSize;
    uint    particleCount;
};

// this is a raw view or the vertex buffer
ByteAddressBuffer buffer : register(t0);


//------------------------------------
// Shared Pipeline Structures

struct VSIn
{
    float3 posSize : TEXCOORD0; // or whatever you like, this is for perf test
    float4 clr     : COLOR;
};

struct VSOut
{
    float2 uv  : TEXCOORD0;
    float4 clr : COLOR;
    float4 pos : SV_Position;
};

// Struct with empty data that won't ever be read.
// Pipelines that utilize on-chip GS memory throw an exceptionally uninformative 
// exception of 'integer division by zero' during pipeline state creation if the 
// preceding pipeline stage specifies void or an empty struct as output.
struct VSNull
{
    float4 _unused : TEXCOORD0;
};

struct VSOutGSIn
{
    float3 posSize : POSSIZE;
    float4 clr     : COLOR;
};

// we haven't got any per vertex hull data, instead we store all of the data in the per-patch
// structure below. the compiler demands the output, so therefore an empty structure
struct HSOutEmpty {};

// we chose to store the original vertex in the per-patch data
// along with compulsory tesselation factors
struct HSPatchData3
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;

    VSOutGSIn vertex;
};

// per-patch data for a quad. again, we store the original vertex in there
// along with the tesselation factors. an interesting side effect here is
// when we set tessfactors to a large value. this way we can get a perfect
// round circle from the quad, or a star.
struct HSPatchData4
{
    float edges[4]  : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;

    VSOutGSIn vertex;
};

struct Empty {};


//------------------------------------
// Shared Functions


float4 NDC(float2 screen)
{
    screen *= invViewportSize * 2;
    screen.x = screen.x - 1;
    screen.y = 1 - screen.y;

    return float4(screen, 0.5f, 1);
}

VSOut VSRender3Helper(VSIn vertex, uint vi)
{
    const float sz = vertex.posSize.z;
    const float2 org = vertex.posSize.xy;

    const float2 verts[3] =
    {
        float2(-0.5f, -0.5f),
        float2(1.5f, -0.5f),
        float2(-0.5f,  1.5f)
    };

    const float2 uvs[3] =
    {
        float2(0.0f, 0.0f),
        float2(0.0f, 2.0f),
        float2(2.0f, 0.0f)
    };

    VSOut v;

    v.uv = uvs[vi];
    v.clr = vertex.clr;
    v.pos = NDC(org + verts[vi] * sz);

    return v;
}

// given the index of the corner and the vertex, move the point to the quad's corner
VSOut VSRender4Helper(VSIn vertex, uint vi)
{
    const float sz = vertex.posSize.z;
    const float2 org = vertex.posSize.xy;

    const float2 verts[6] =
    {
        float2(0, 0),
        float2(1, 0),
        float2(1, 1),
        float2(1, 1),
        float2(0, 1),
        float2(0, 0),
    };

    VSOut v;

    v.uv = verts[vi];
    v.clr = vertex.clr;
    v.pos = NDC(org + (verts[vi] - 0.5f) * sz);

    return v;
}

// given the index of the corner and the vertex, move the point to the quad's corner
VSOut VSRenderNativeQuadHelper(VSIn vertex, uint vi)
{
    const float sz = vertex.posSize.z;
    const float2 org = vertex.posSize.xy;

    const float2 verts[4] =
    {
        float2(0, 0),
        float2(1, 0),
        float2(1, 1),
        float2(0, 1)
    };

    VSOut v;

    v.uv = verts[vi];
    v.clr = vertex.clr;
    v.pos = NDC(org + (verts[vi] - 0.5f) * sz);

    return v;
}

// the vertex buffer has been created with a raw byte view which allows us to read
// the data manually, similar to vfetch on 360
// the reads can only be 32 bit unsigned integers so it's a bit longwinded
// however, asfloat is a no-op in gpu instructions so this simply converts into
// two tbuffer read instruction
VSIn ReadVertex(uint sourceIndex)
{
    VSIn vertex;

    // asfloat is required here because the data is read as uint from a raw buffer
    vertex.posSize = asfloat(buffer.Load3(sourceIndex * 28 + 0));
    vertex.clr = asfloat(buffer.Load4(sourceIndex * 28 + 12));

    return vertex;
}
