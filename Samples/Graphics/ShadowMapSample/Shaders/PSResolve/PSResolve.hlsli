//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../Common.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Different items expected to be set by the command line
//-------------------------------------------------------------------------------------------------------------
#ifndef RemappingFunction
#define RemappingFunction Nop
#endif
#ifndef BlurOutputType
#define BlurOutputType float
#endif


ConstantBuffer<CBRemapStruct> cbRemapping : register(b2);

Texture2DMS<float> g_texMSResolve : register(t0);


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
    return fIn / (cbRemapping.g_fFarNearRatio - (cbRemapping.g_fFarNearRatio - 1.0) * fIn);
}


//-------------------------------------------------------------------------------------------------------------
// Take x and x^2 of the input
//-------------------------------------------------------------------------------------------------------------
float2 RemappingFunctionVariance( float fIn )
{
    fIn = LinearizeDepth( fIn );
    return float2( fIn, Square( fIn ) );
}


//-------------------------------------------------------------------------------------------------------------
// Take exp2 of the input, as required by Exponential shadow mapping.
//
// Also, scale and offset the input so as to use as much as possible of the range of float32.
// This should ideally be 254.0f * fIn - 127.0f, to match the exponent range [-127,127], not including
// denorms.  However, there are apparently reasons to use less than the full range (???).  Perhaps this
// has to do with not underflowing/overflowing during filtering.
//-------------------------------------------------------------------------------------------------------------
float RemappingFunctionExponential( float fIn )
{
    fIn = LinearizeDepth( fIn );
    return exp2( 252.0f * fIn - 126.0f );
}


//-------------------------------------------------------------------------------------------------------------
// Take exp2, exp2^2, -exp2, -exp2^2 of the input, as required by Exponential Variance shadow mapping.
//
// Also, scale and offset the input so as to use as much as possible of the range of float32.
// This should be half of what we use for Exponential, to accommodate the square for Variance.
//
// TODO option to use float16 here.  The quality is definitely poorer, but still passable.
//-------------------------------------------------------------------------------------------------------------
float4 RemappingFunctionExponentialVariance( float fIn )
{
    fIn = LinearizeDepth( fIn );
    float fExponent = ( 252.0f * fIn - 126.0f ) / 2.0f;
    float fExponentialPlus = exp2( fExponent );
    float fExponentialMinus = -exp2( -fExponent );
    return float4( fExponentialPlus, Square( fExponentialPlus ), fExponentialMinus, Square( fExponentialMinus ) );
}


//-------------------------------------------------------------------------------------------------------------
// Resolve a multisampled depth buffer into a filterable shadow map
//-------------------------------------------------------------------------------------------------------------
[RootSignature(ROOT_SIG)]
BlurOutputType main(InterpolantsTexcoord In) : SV_TARGET0
{
    BlurOutputType fOutput = 0.0f;  
    uint2 viDims; 
    uint iSampleCount;
    g_texMSResolve.GetDimensions( viDims.x, viDims.y, iSampleCount );
    float fWeight = 1.0f / iSampleCount;    // avoid overflow in the case of exponential remapping

    for( uint iSample = 0; iSample < iSampleCount; ++iSample )
    {
        fOutput += fWeight * RemappingFunction( g_texMSResolve.Load( floor( viDims * In.texcoord ), iSample ) );
    }

    return fOutput;
}
