//FidelityFX Variable Rate Sample
//
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
// Licensed under the MIT License

#define VariableShadingOverlayRS \
        "RootFlags(DENY_VERTEX_SHADER_ROOT_ACCESS),"\
        "CBV(b0, visibility=SHADER_VISIBILITY_PIXEL),"\
        "DescriptorTable(SRV(t0 , numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL),"\

static const uint FFX_VARIABLESHADING_RATE1D_1X = 0x0;
static const uint FFX_VARIABLESHADING_RATE1D_2X = 0x1;
static const uint FFX_VARIABLESHADING_RATE1D_4X = 0x2;
#define FFX_VARIABLESHADING_MAKE_SHADING_RATE(x,y) ((x << 2) | (y))

static const uint FFX_VARIABLESHADING_RATE_1X1 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_1X, FFX_VARIABLESHADING_RATE1D_1X); // 0;
static const uint FFX_VARIABLESHADING_RATE_1X2 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_1X, FFX_VARIABLESHADING_RATE1D_2X); // 0x1;
static const uint FFX_VARIABLESHADING_RATE_2X1 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_2X, FFX_VARIABLESHADING_RATE1D_1X); // 0x4;
static const uint FFX_VARIABLESHADING_RATE_2X2 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_2X, FFX_VARIABLESHADING_RATE1D_2X); // 0x5;
static const uint FFX_VARIABLESHADING_RATE_2X4 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_2X, FFX_VARIABLESHADING_RATE1D_4X); // 0x6;
static const uint FFX_VARIABLESHADING_RATE_4X2 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_4X, FFX_VARIABLESHADING_RATE1D_2X); // 0x9;
static const uint FFX_VARIABLESHADING_RATE_4X4 = FFX_VARIABLESHADING_MAKE_SHADING_RATE(FFX_VARIABLESHADING_RATE1D_4X, FFX_VARIABLESHADING_RATE1D_4X); // 0xa;

// Constant Buffer
cbuffer FFX_Variable_Shading_CB0
{
    int2    g_Resolution;
    uint    g_TileSize;
    float   g_VarianceCutoff;
    float   g_MotionFactor;
}

Texture2D<uint> inU8 : register(t0);

struct VERTEX_OUT
{
    float4 vPosition : SV_POSITION;
};

[RootSignature(VariableShadingOverlayRS)]
VERTEX_OUT mainVS(uint id : SV_VertexID)
{
    VERTEX_OUT output;
    output.vPosition = float4(float2(id & 1, id >> 1) * float2(4, -4) + float2(-1, 1), 0, 1);
    return output;
}

[RootSignature(VariableShadingOverlayRS)]
float4 mainPS(VERTEX_OUT input) : SV_Target
{
    int2 pos = input.vPosition.xy / g_TileSize;
    // encode different shading rates as colors
    float3 color = float3(1, 1, 1);

    switch (inU8[pos].r)
    {
    case FFX_VARIABLESHADING_RATE_1X1:
        color = float3(0.5, 0.0, 0.0);
        break;
    case FFX_VARIABLESHADING_RATE_1X2:
        color = float3(0.5, 0.5, 0.0);
        break;
    case FFX_VARIABLESHADING_RATE_2X1:
        color = float3(0.5, 0.25, 0.0);
        break;
    case FFX_VARIABLESHADING_RATE_2X2:
        color = float3(0.0, 0.5, 0.0);
        break;
    case FFX_VARIABLESHADING_RATE_2X4:
        color = float3(0.25, 0.25, 0.5);
        break;
    case FFX_VARIABLESHADING_RATE_4X2:
        color = float3(0.5, 0.25, 0.5);
        break;
    case FFX_VARIABLESHADING_RATE_4X4:
        color = float3(0.0, 0.5, 0.5);
        break;
    }

    // add grid
    int2 grid = int2(input.vPosition.xy) % g_TileSize;
    bool border = (grid.x == 0) || (grid.y == 0);

    return float4(color, 0.5) * (border ? 0.5f : 1.0f);
}
