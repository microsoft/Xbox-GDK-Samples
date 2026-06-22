//--------------------------------------------------------------------------------------
// MinLODFilterEmulation.hlsli
//
// HLSL code that emulates a MinLOD map hardware sampling filter.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

float SampleMinLODFilter2D(Texture2D sampledtex, Texture2D minlodtex, float2 TexCoord, SamplerState samp, float4 FilterSlopes)
{
    uint2 MinLODDimensions;
    minlodtex.GetDimensions(MinLODDimensions.x, MinLODDimensions.y);

    // Compute the MinLOD map aspect ratio based on the primary map dimensions,
    // and apply the aspect ratio to the filter slopes.
    // Note that it would be ideal for this aspect ratio to be computed and stored 
    // when the view descriptor for the MinLOD map is generated.
    uint2 PrimaryDimensions;
    sampledtex.GetDimensions(PrimaryDimensions.x, PrimaryDimensions.y);

    // Primary map dimensions *should* be an integer multiple (probably power of 2?)
    // of residency map dimensions.
    uint2 MinLODAspect = PrimaryDimensions / MinLODDimensions;

    int slope_u = FilterSlopes.x;
    int slope_v = FilterSlopes.y;
    if (MinLODAspect.x > MinLODAspect.y)
    {
        slope_u *= (MinLODAspect.x / MinLODAspect.y);
    }
    else
    {
        slope_v *= (MinLODAspect.y / MinLODAspect.x);
    }

    float2 AbsoluteCoords = (float2)MinLODDimensions * TexCoord;

    // It turns out that the float precision on texel selection doesn't perfectly match the nearest_texel logic
    // below that uses 0.5 as the cutoff between texels. Clamp the texcoords used by the gather to texel increments.
    float2 ClampedCoords = round(AbsoluteCoords) / (float2)MinLODDimensions;
    float4 RawSamples = minlodtex.Gather(samp, ClampedCoords).zwyx;

    float u_frac = frac(AbsoluteCoords.x);
    float v_frac = 1 - frac(AbsoluteCoords.y);

    float u_ofs = FilterSlopes.z;
    float v_ofs = FilterSlopes.w;

    int nearest_texel = ((u_frac < 0.5) ? 0 : 1) | ((v_frac < 0.5) ? 0 : 2);
    float gradient_u = (u_frac < 0.5 ? u_frac - u_ofs : 1 - u_frac - u_ofs);
    float gradient_v = (v_frac < 0.5 ? v_frac - v_ofs : 1 - v_frac - v_ofs);
    float weight_u = clamp(gradient_u * slope_u, 0, 1);
    float weight_v = clamp(gradient_v * slope_v, 0, 1);

    float4 NewSamples;
    NewSamples[0] = RawSamples[nearest_texel];
    NewSamples[1] = max(RawSamples[nearest_texel & 2], RawSamples[(nearest_texel & 2) | 1]);
    NewSamples[2] = max(RawSamples[nearest_texel & 1], RawSamples[(nearest_texel & 1) | 2]);
    NewSamples[3] = max(RawSamples[0], max(RawSamples[1], max(RawSamples[2], RawSamples[3])));

    float result = NewSamples[0] * weight_u*weight_v + 
                   NewSamples[1] * (1 - weight_u)*weight_v + 
                   NewSamples[2] * weight_u*(1 - weight_v) + 
                   NewSamples[3] * (1 - weight_u)*(1 - weight_v);

    return result;
}
