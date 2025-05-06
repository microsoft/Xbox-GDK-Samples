// FSR Sample
//
// Copyright(c) 2021 Advanced Micro Devices, Inc.All rights reserved.
// Licensed under the MIT License

#define WIDTH 64
#define HEIGHT 1
#define DEPTH 1 

// A basic bilinear upsample to compare against

#define ComputeRS \
        "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(SRV(t0 , numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
            "StaticSampler(s0,"\
        "           filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
        "           addressU = TEXTURE_ADDRESS_CLAMP,"\
        "           addressV = TEXTURE_ADDRESS_CLAMP,"\
        "           addressW = TEXTURE_ADDRESS_CLAMP,"\
        "           borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
        "           maxAnisotropy = 1,"\
        "           comparisonFunc = COMPARISON_ALWAYS,"\
        "           visibility = SHADER_VISIBILITY_ALL )"


cbuffer cb : register(b0)
{
    uint4 Const0;
    uint4 Const1;
};

SamplerState samLinearClamp : register(s0);

Texture2D InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

void BilinearFilter(int2 pos)
{
    float2 pp = float2(pos) * asfloat(Const0.xy) * asfloat(Const1.xy) + float2(0.5, -0.5) * asfloat(Const1.zw);
    OutputTexture[pos] = InputTexture.SampleLevel(samLinearClamp, pp, 0.0);
}

uint ABfe(uint src,uint off,uint bits){uint mask=(1u<<bits)-1;return (src>>off)&mask;}
uint ABfiM(uint src,uint ins,uint bits){uint mask=(1u<<bits)-1;return (ins&mask)|(src&(~mask));}
uint2 ARmp8x8(uint a){return uint2(ABfe(a,1u,3u),ABfiM(ABfe(a,3u,3u),a,1u));}

[RootSignature(ComputeRS)]
[numthreads(WIDTH, HEIGHT, DEPTH)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
    // Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
    uint2 gxy = ARmp8x8(LocalThreadId.x) + uint2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
    BilinearFilter(gxy);
    gxy.x += 8u;
    BilinearFilter(gxy);
    gxy.y += 8u;
    BilinearFilter(gxy);
    gxy.x -= 8u;
    BilinearFilter(gxy);
}

