//--------------------------------------------------------------------------------------
// PsCommon.hlsli
//
// Common code for MSAA/EQAA visualizations
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "EQAA.hlsli"

Texture2D texFragment   : register(t1);
Texture2D texSample     : register(t2);

struct InterpolantsTexturePerSample
{
    float4 position         : SV_POSITION0;
    sample float2 texcoord  : TEXCOORD0;    // The 'sample' modifier means interpolation is per-sample when running at sample frequency
    unsigned int sample : SV_SampleIndex;
};

Texture2DMS<float4> texMS : register(t0);

#if ENABLE_EQAA
Texture2D<uint2> texFMask : register(t1);

#include "FMask.hlsli"
#endif

//-------------------------------------------------------------------------------------------------------------
// Name: CrossHatch()
// Desc: Draw a cross-hatch pattern in screen space to mark unknown samples
//-------------------------------------------------------------------------------------------------------------
float4 CrossHatch(float2 screenCoords)
{
    int2 screenCoordsUnnormalized = floor(screenCoords);
    bool parity = (((screenCoordsUnnormalized.x + screenCoordsUnnormalized.y) ^ (screenCoordsUnnormalized.x - screenCoordsUnnormalized.y)) >> 3) & 1;

    float4 black = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 white = float4(1.0f, 1.0f, 1.0f, 1.0f);

    return parity ? black : white;
}


//-------------------------------------------------------------------------------------------------------------
// Name: LoadFromTexMS()
// Desc: Replacement for Texture2D::Load which can do FMask decode if necessary
//-------------------------------------------------------------------------------------------------------------
float4 LoadFromTexMS(uint2 texcoord,
    uniform uint sample,
    float2 screenCoords)
{
#if ENABLE_EQAA
    if (!COLOR_EXPANDED)
    {
        bool unknown;
        float4 color = ManualLoad(texMS, texFMask, texcoord, sample, unknown);
        return unknown ? CrossHatch(screenCoords) : color;
    }
    else
#endif
    {
        return texMS.Load(texcoord, sample);
    }
}
