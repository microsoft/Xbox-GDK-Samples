//--------------------------------------------------------------------------------------
// RGB2NV12.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define ROOT_SIGNATURE \
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), \
     DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), \
     DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), \
     StaticSampler(s0, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, \
                   comparisonFunc = COMPARISON_NEVER, borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK, filter = FILTER_MIN_MAG_LINEAR_MIP_POINT )"

Texture2D<float4>   RGBTexture  : register(t0);     // Source RGB texture
RWTexture2D<float>  YBuffer     : register(u0);     // Full resolution luma
RWTexture2D<float2> UVBuffer    : register(u1);     // Down sampled half resolution chroma
SamplerState        Sampler     : register(s0);     // Bilinear sampler

// YUV conversion for Rec.709
float Y(float3 rgb) { return dot(float3( 0.2126f,  0.7152f,  0.0722f ), rgb); }
float U(float3 rgb) { return dot(float3(-0.1146f, -0.3854f,  0.5000f ), rgb); }
float V(float3 rgb) { return dot(float3( 0.5000f, -0.4542f, -0.0458f ), rgb); }

static const float2 InvDestDims = 1.0f / float2(1920.0f, 1080.0f);

float3 DownSampleBilinear(uint2 id)
{
    float2 loc = id.xy + float2(0.25f, 0.25f);

    float2 uv = loc * InvDestDims;
    float3 color = RGBTexture.SampleLevel(Sampler, uv, 0).rgb;

    uv = (loc + float2(0.5f, 0.0f)) * InvDestDims;
    color += RGBTexture.SampleLevel(Sampler, uv, 0).rgb;

    uv = (loc + float2(0.0f, 0.5f)) * InvDestDims;
    color += RGBTexture.SampleLevel(Sampler, uv, 0).rgb;

    uv = (loc + float2(0.5f, 0.5f)) * InvDestDims;
    color += RGBTexture.SampleLevel(Sampler, uv, 0).rgb;

    color *= 0.25f;

    return color;
}

[numthreads(8, 8, 1)]
[RootSignature(ROOT_SIGNATURE)]
void main(uint3 id : SV_DispatchThreadID)
{
    float3 rgb = RGBTexture[id.xy].rgb;

    // Gamma correct before encoding
    rgb = pow(rgb, 1.0f/2.2f);

    // Luma
    YBuffer[id.xy] = Y(rgb) + (16.0f / 256.0f);

    // Chroma at half resolution
    if ((id.x % 2 == 1) && (id.y % 2 == 1))
    {
        float3 downSampled = DownSampleBilinear(id.xy);

        // Gamma correct before encoding
        downSampled = pow(downSampled, 1.0f/2.2f);

        float2 chroma = float2(U(downSampled), V(downSampled));
        UVBuffer[id.xy / 2] = chroma + (128.0f / 256.0f);
    }
}
