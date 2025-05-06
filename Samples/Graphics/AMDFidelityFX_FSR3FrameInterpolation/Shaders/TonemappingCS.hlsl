// AMD Cauldron code
// 
// Copyright(c) 2020 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "tonemappers.hlsli"

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    float u_exposure : packoffset(c0.x);
    int   u_toneMapper : packoffset(c0.y);
    float fsr_scale : packoffset(c0.z);
    int  gamma2 : packoffset(c0.w);
}

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
RWTexture2D<float4>      input              :register(u0);
RWTexture2D<float4>      output              :register(u1);

SamplerState     samLinear   :register(s0);

float3 Tonemap(float3 color, float exposure, int tonemapper)
{
    color *= exposure;

    switch (tonemapper)
    {
    case 0: return TimothyTonemapper(color);
    case 1: return DX11DSK(color);
    case 2: return Reinhard(color);
    case 3: return Uncharted2Tonemap(color);
    case 4: return ACESFilm(color);
    case 5: return color;
    default: return float3(1, 1, 1);
    }
}

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------

#define ComputeRS \
        "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
        "StaticSampler(s0,"\
        "           filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
        "           addressU = TEXTURE_ADDRESS_CLAMP,"\
        "           addressV = TEXTURE_ADDRESS_CLAMP,"\
        "           addressW = TEXTURE_ADDRESS_CLAMP,"\
        "           borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
        "           maxAnisotropy = 1,"\
        "           comparisonFunc = COMPARISON_ALWAYS,"\
        "           visibility = SHADER_VISIBILITY_ALL )"

#define WIDTH 8
#define HEIGHT 8
#define DEPTH 1

[RootSignature(ComputeRS)]
[numthreads(WIDTH, HEIGHT, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
    int2 coord = dtID.xy;

    float4 texColor = input.Load(coord);

    float3 color = Tonemap(texColor.rgb, u_exposure, u_toneMapper);

    if (gamma2 != 0)
    {
        // Gamma2 conversion
        color = sqrt(color);
    }
    output[coord] = float4(color, texColor.a);
}
