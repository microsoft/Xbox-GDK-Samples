//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../Common.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Different items expected to be set by the command line
//-------------------------------------------------------------------------------------------------------------
#ifndef g_iBlurKernelSize
#define g_iBlurKernelSize 2
#endif
#ifndef g_iOffsetX
#define g_iOffsetX 0
#endif
#ifndef g_iOffsetY
#define g_iOffsetY 0
#endif
#ifndef g_bUseBilinearFiltering
#define g_bUseBilinearFiltering false
#endif
#ifndef CalculateBlurWeights
#define CalculateBlurWeights CalculateWeightsSquare
#endif
#ifndef RemappingFunction
#define RemappingFunction Nop
#endif
#ifndef BlurInputType
#define BlurInputType float
#endif
#ifndef BlurOutputType
#define BlurOutputType float
#endif


ConstantBuffer<CBRemapStruct> cbRemapping : register(b2);
Texture2D<BlurInputType> g_texFilter : register(t0);


// SamplerStates
SamplerState g_samFilter : register(s0);



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


#define MAX_WEIGHTS 16
//-------------------------------------------------------------------------------------------------------------
// Calculate filter weights for a square filter
//-------------------------------------------------------------------------------------------------------------
void CalculateWeightsSquare( inout float Weights[ MAX_WEIGHTS ], in uniform int iNumWeights )
{
    float fNormalizer = 1.0f / iNumWeights; 

    [unroll(64)]
    for( int i = 0; i < iNumWeights + 0; ++i )
    {
        Weights[ i ] = fNormalizer;
    }
}


//-------------------------------------------------------------------------------------------------------------
// Calculate filter weights for a Gaussian filter
//-------------------------------------------------------------------------------------------------------------
void CalculateWeightsGaussian( inout float Weights[ MAX_WEIGHTS ], in uniform int iNumWeights )
{
    // Hopefully, this will be compiled away into literals for small kernel sizes.  We have to be
    // careful to only use weights < 1.0f, because of overflow concerns with exponential shadow maps.
    float fNormalizer = exp2( -iNumWeights + 1 ); 

    [unroll(64)]
    for( int i = 0; i < iNumWeights + 0; ++i )
    {
        Weights[ i ] = fNormalizer;

        [unroll(64)]
        for( int j = i - 1; j > 0; --j )
        {
            Weights[ j ] += Weights[ j - 1 ];
        }
        Weights[ 0 ] = fNormalizer;
    }
}


//-------------------------------------------------------------------------------------------------------------
// Perform a blur in a particular direction. This function can be optimized in these ways:
//  - Under appropriate conditions, we can use bilinear filtering (done), which can speed the operation up by 2x.
//  - We could also use Gather to speed up axis-aligned downsamples, by retrieving 4 samples at once.
//  - For large kernel sizes, something like the WidthIndependentGaussianBlur sample would be fastest.
//-------------------------------------------------------------------------------------------------------------
[RootSignature(ROOT_SIG)]
BlurOutputType main(InterpolantsTexcoord In) : SV_TARGET0
{
    float Weights[ MAX_WEIGHTS ] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    CalculateBlurWeights( Weights, g_iBlurKernelSize );
    Weights[ g_iBlurKernelSize ] = 0.0; // accommodate odd kernel sizes by adding a 0 

    BlurOutputType fOutput = 0.0f;

    if( g_bUseBilinearFiltering ) 
    {
        uint2 viDims = 0;
        g_texFilter.GetDimensions( viDims.x, viDims.y );

        [unroll(64)]
        for( int iStep = 0; iStep < g_iBlurKernelSize; iStep += 2 )
        {
            float fTwoWeights = Weights[ iStep ] + Weights[ iStep + 1 ];
            float fWeightLerpFactor = fTwoWeights == 0.0f ? 0.0f : Weights[ iStep + 1 ] / fTwoWeights;
            float fOffset = iStep + fWeightLerpFactor - (g_iBlurKernelSize / 2);
            fOutput += fTwoWeights * g_texFilter.Sample(g_samFilter, In.texcoord + (fOffset * int2(g_iOffsetX, g_iOffsetY)) / viDims);
        }
    }
    else 
    {
        [unroll(64)]
        for( int iStep = 0; iStep < g_iBlurKernelSize; ++iStep )
        {
            int iOffset = iStep - g_iBlurKernelSize / 2;
            int OffsetX = iOffset * g_iOffsetX;
            int OffsetY = iOffset * g_iOffsetY;
            const int2 offsets = int2(OffsetX, OffsetY);
            BlurOutputType remapOutput = RemappingFunction(g_texFilter.Sample(g_samFilter, In.texcoord, offsets));
            fOutput += Weights[iStep] * remapOutput;
        }
    }

    return fOutput;
}
