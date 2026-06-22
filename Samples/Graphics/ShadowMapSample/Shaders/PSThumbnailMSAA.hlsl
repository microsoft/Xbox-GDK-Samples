#include "Common.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Different items expected to be set by the command line
//-------------------------------------------------------------------------------------------------------------
#ifndef RescaleThumbnail
#define RescaleThumbnail Nop
#endif


//-------------------------------------------------------------------------------------------------------------
// Convert perspective depth back into normalized linear depth.  This conversion is unnecessary for PCF:  
// All we care about in that technique is whether one depth is > another, and perspective mapping is monotonic.
//
// For other techniques, conversion is not mathematically required, but does affect the outcome, since these
// techinques are based on approximations, whose effectiveness depends on the depth distribution.
//
// For visualization, linear depth is best, as perspective depth tends to concentrate the whole scene near
// one end of the range.
//-------------------------------------------------------------------------------------------------------------
float LinearizeDepth( float fIn )
{
    return fIn / ( g_fFarNearRatio - ( g_fFarNearRatio - 1.0 ) * fIn );
}


//-------------------------------------------------------------------------------------------------------------
// Pixel shader which resolves MSAA depth and displays it as color
//-------------------------------------------------------------------------------------------------------------
Texture2DMS<float4> g_texMSThumb : register(t0);

[RootSignature(ROOT_SIG)]
float4 main(InterpolantsTexcoord In) : SV_TARGET0
{
    float vOutput = 0.0f;
    uint2 viDims; 
    uint iSampleCount;
    g_texMSThumb.GetDimensions( viDims.x, viDims.y, iSampleCount );

    for( uint iSample = 0; iSample < iSampleCount; ++iSample )
    {
        vOutput += LinearizeDepth( g_texMSThumb.Load( floor( viDims * In.texcoord ), iSample ).x );
    }

    return vOutput / iSampleCount;
}
