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

#if defined __XBOX_ONE || defined __XBOX_SCARLETT
#include "HDRCommon.hlsli"

// Define how bright white is in HDR
static const float PaperWhiteNits = 250.0f;
#endif

//--------------------------------------------------------------------------------------
// Constant Buffer
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register(b0)
{
    int u_displayMode;
}

//--------------------------------------------------------------------------------------
// I/O Structures
//--------------------------------------------------------------------------------------
struct VERTEX
{
    float2 vTexcoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Texture definitions
//--------------------------------------------------------------------------------------
Texture2D        sceneTexture     : register(t0);
SamplerState     samLinearWrap    : register(s0);

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
#define DrawRS \
        "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
        "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
        "StaticSampler(s0,"\
        "           filter = FILTER_MIN_MAG_LINEAR_MIP_POINT,"\
        "           addressU = TEXTURE_ADDRESS_CLAMP,"\
        "           addressV = TEXTURE_ADDRESS_CLAMP,"\
        "           addressW = TEXTURE_ADDRESS_CLAMP,"\
        "           borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,"\
        "           maxAnisotropy = 1,"\
        "           comparisonFunc = COMPARISON_ALWAYS,"\
        "           visibility = SHADER_VISIBILITY_ALL )"

[RootSignature(DrawRS)]
float4 main(VERTEX Input) : SV_Target
{
    float4 color = sceneTexture.Sample(samLinearWrap, Input.vTexcoord);

    // Gamma2 conversion
    color *= color;

    switch (u_displayMode)
    {
        case 0:
            // SDR
            // this shader is a NOP for sdr
            
            break;

#if defined __XBOX_ONE || defined __XBOX_SCARLETT
        case 1:
        {
            // Brightness adjustment for HDR
            color *= 4.0f;

            // HDR10_ST2084
            color.xyz = ConvertToHDR10(color, PaperWhiteNits).xyz;
            break;
        }
#endif
    }

    return float4(color.xyz, color.a);
}
