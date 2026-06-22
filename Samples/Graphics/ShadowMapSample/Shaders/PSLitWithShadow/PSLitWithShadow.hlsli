//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "../Common.hlsli"

// SamplerStates
#ifdef SUPPORT_NORMAL_MAP
SamplerState g_samNormal            : register( s0 );
#else
SamplerState g_samDiffuse           : register( s0 );
#endif
SamplerState g_samPointClamp        : register( s1 );
SamplerState g_samVariance          : register( s1 );
SamplerState g_samExponential       : register( s1 );
SamplerComparisonState g_samCompPCF : register( s1 );

//-------------------------------------------------------------------------------------------------------------
// Different items expected to be set by the command line
//-------------------------------------------------------------------------------------------------------------
#ifndef CalcUnshadowedAmount
#define CalcUnshadowedAmount CalcUnshadowedAmountPoint
#endif
#ifndef g_iBlurKernelSize
#define g_iBlurKernelSize 2
#endif
#ifndef CalculateBlurWeights
#define CalculateBlurWeights CalculateWeightsSquare
#endif
#ifndef RemappedType
#define RemappedType float
#endif

ConstantBuffer<CBLightStruct> cbLight               : register( b1 );
ConstantBuffer<CBRemapStruct> cbRemapping           : register( b2 );
Texture2D<RemappedType> g_texShadow[g_iNumLights]   : register( t0 ); // the mesh renderer consumes 0-2
Texture2D<float4> g_texDiffuse                      : register( t1 );
#ifdef SUPPORT_NORMAL_MAP
Texture2D<float3> g_texNormal                       : register( t2 );
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
// Lerp weights to account for sub-texel offset.  If we are at a texel center, this is a Nop.  
//
// Note the precomputed filter weights could be placed in a constant buffer or hard-coded, but the output
// of this function varies per pixel.
//-------------------------------------------------------------------------------------------------------------
void LerpWeights( inout float Weights[ MAX_WEIGHTS ], in int iNumWeightsIn, inout float2 WeightsLerped[ MAX_WEIGHTS ], in float2 vLerp )
{
    // Shift the weights to put a zero on either end
    float WeightsShifted[ MAX_WEIGHTS ];
    WeightsShifted[ 0 ] = 0.0f;
    [unroll(64)]
    for( int i = 1; i < iNumWeightsIn; ++i )
    {
        WeightsShifted[ i ] = Weights[ i - 1 ];
    }
    WeightsShifted[ iNumWeightsIn ] = 0.0f;

    // Lerp the weights to account for sub-texel offset
    [unroll(64)]
    for( int j = 0; j < iNumWeightsIn; ++j )
    {
        WeightsLerped[ j ] = lerp( WeightsShifted[ j + 1 ], WeightsShifted[ j + 0 ], vLerp );
    }
}


#ifdef SUPPORT_NORMAL_MAP
//-------------------------------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform
//-------------------------------------------------------------------------------------------------------------
float3 CalcPerPixelNormal( in float2 vTexcoord, in float3 vVertNormal, in float3 vVertTangent )
{
	// Compute tangent frame
	vVertNormal =   normalize( vVertNormal );	
	vVertTangent =  normalize( vVertTangent );	
	float3 vVertBinormal = normalize( cross( vVertTangent, vVertNormal ) );	
	float3x3 mTangentSpaceToWorldSpace = float3x3( vVertTangent, vVertBinormal, vVertNormal ); 
	
	// Compute per-pixel normal
	float3 vBumpNormal = g_texNormal.Sample( g_samNormal, vTexcoord ).xyz;
	vBumpNormal = 2.0f * vBumpNormal - 1.0f;
	
	return mul( vBumpNormal, mTangentSpaceToWorldSpace );
}
#endif


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using point sampling
// Returns: 0 --> in full shadow
//          1 --> fully lit
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountPoint( Texture2D<float> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    return texShadow.Sample( g_samPointClamp, vShadowTexCoord ) >= fLightSpaceDepth;
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using percentage-closer filtering
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// Uses SampleCmp (shader model 4) to perform 4 shadow map tests at once, and filter the results.  SampleCmp
// has the disadvantage that if you sample at an exact pixel center, you only get 1 shadow map tap.  You can 
// avoid this issue by sampling at 1 texel offsets, as is done here, but that largely defeats the purpose of 
// using SampleCmp rather than point-sampling.  
// 
// Two generally better approaches are shown below: CalcUnshadowedAmountPCFSampleCmpStep1 and 
// CalcUnshadowedAmountPCFGatherCmp.  These use fewer taps, but more ALU.
//
// For best results, it is desirable for shadow receivers *not* to be rendered into the shadow map.  This 
// eliminates much of the need for depth bias to avoid self-shadowing.
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountPCFSampleCmpStep1( Texture2D<float> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    // The first tap hits 2x2 texels, and subsequent taps shift by 1 texel in x or y
    const int iNumSamples = max( 1, g_iBlurKernelSize - 1 );
    float Weights[ MAX_WEIGHTS ];
    CalculateBlurWeights( Weights, iNumSamples );

    float fUnshadowedAmount = 0.0f;

    [unroll(64)]
    for( int iStepX = 0; iStepX < iNumSamples; ++iStepX )
    {
        float fWeightX = Weights[ iStepX ];
        int iOffsetX = iStepX - iNumSamples / 2;
        [unroll(64)]
        for( int iStepY = 0; iStepY < iNumSamples; ++iStepY )
        {
            float fWeightY = Weights[ iStepY ];
            int iOffsetY = iStepY - iNumSamples / 2;
            fUnshadowedAmount += fWeightX * fWeightY * 
                texShadow.SampleCmp( g_samCompPCF, vShadowTexCoord, fLightSpaceDepth, int2( iOffsetX, iOffsetY ) );
        }
    }

    return fUnshadowedAmount;
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using percentage-closer filtering
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// Uses SampleCmp (shader model 4) to perform 4 shadow map tests at once, and filter the results.  In order
// to avoid repeated sampling of the same texel, we sample at offsets of 2 texels.  In order to get the proper
// weights, we shift the shadow map UVs.  This will cost some ALU, and also make it impossible to use
// integer offsets to the sample instruction.
// 
// This will also screw up trilinear and aniso filtering, for what it's worth.
//
// For best results, it is desirable for shadow receivers *not* to be rendered into the shadow map.  This 
// eliminates much of the need for depth bias to avoid self-shadowing.
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountPCFSampleCmpStep2( Texture2D<float> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    const int iBlurKernelSizePadded = g_iBlurKernelSize + 1;    // accommodate odd kernel size

    float Weights[ MAX_WEIGHTS ];
    CalculateBlurWeights( Weights, g_iBlurKernelSize - 1 );
    Weights[ g_iBlurKernelSize - 1 ] = 0.0f;

    // Lerp the weights to account for incoming sub-texel offset
    uint2 viShadowMapDims;
    texShadow.GetDimensions( viShadowMapDims.x, viShadowMapDims.y );
    float2 vSubTexelCoord = frac( vShadowTexCoord * viShadowMapDims + 0.5f );
    float2 vSnappedTexCoord = floor( vShadowTexCoord * viShadowMapDims - 0.5f ) + 0.5f;
    float2 WeightsLerped[ MAX_WEIGHTS ] =
    {
        float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0),
        float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0), float2(0, 0)
    };
    LerpWeights( Weights, g_iBlurKernelSize, WeightsLerped, vSubTexelCoord );

    // Adjust the texcoords and weights so that bilinear filtering produces the right outcome
    const int iNumSamples = iBlurKernelSizePadded / 2;
    float2 DoubleWeights[ iNumSamples ];
    float2 SubTexelOffsets[ iNumSamples ];
    for( int i = 0; i < iNumSamples; ++i )
    {
        DoubleWeights[ i ] = WeightsLerped[ 2 * i ] + WeightsLerped[ 2 * i + 1 ];
#if __HLSL_VERSION >= 2019
        float2 LerpFactor = select(DoubleWeights[i] == 0.0f, 0.0f, WeightsLerped[2 * i + 1]) / DoubleWeights[i];
#else
        float2 LerpFactor = DoubleWeights[ i ] == 0.0f ? 0.0f : WeightsLerped[ 2 * i + 1 ] / DoubleWeights[ i ];
#endif
        SubTexelOffsets[ i ] = 2 * i + LerpFactor - ( g_iBlurKernelSize - 1 ) / 2;
    }

    float fUnshadowedAmount = 0.0f;

    // Sum up the samples
    [unroll(64)]
    for( int iStepX = 0; iStepX < iNumSamples; ++iStepX )
    {
        [unroll(64)]
        for( int iStepY = 0; iStepY < iNumSamples; ++iStepY )
        {
            float2 vSubTexelOffset = float2( SubTexelOffsets[ iStepX ].x, SubTexelOffsets[ iStepY ].y );
            fUnshadowedAmount += DoubleWeights[ iStepX ].x * DoubleWeights[ iStepY ].y * 
                texShadow.SampleCmp( g_samCompPCF, ( vSnappedTexCoord + vSubTexelOffset ) / viShadowMapDims, fLightSpaceDepth );
        }
    }

    return fUnshadowedAmount;
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using percentage-closer filtering
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// Uses GatherCmp (shader model 5) to perform 4 shadow map tests at once, and retrieve the results.  You must
// filter the results manually using shader instructions. Because the texcoords are not perturbed here, the 
// samples use integer offsets, which is better for codegen.
//
// For best results, it is desirable for shadow receivers *not* to be rendered into the shadow map.  This 
// eliminates the need for depth bias to avoid self-shadowing.
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountPCFGatherCmp( Texture2D<float> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    const int iBlurKernelSizePadded = g_iBlurKernelSize + 1;    // accommodate odd kernel size
    const int iNumSamples = iBlurKernelSizePadded / 2;
    
    uint2 viShadowMapDims;
    texShadow.GetDimensions( viShadowMapDims.x, viShadowMapDims.y );
    float2 vSubTexelCoord = frac( vShadowTexCoord * viShadowMapDims + 0.5f );
    float2 vSnappedTexCoord = floor( vShadowTexCoord * viShadowMapDims - 0.5f ) + 0.5f;
    
    // Fetch g_iBlurKernelSize x g_iBlurKernelSize bool shadow test results
    bool ShadowTestResults[ iBlurKernelSizePadded ][ iBlurKernelSizePadded ];
    [unroll(64)]
    for( int iStepX = 0; iStepX < iNumSamples; ++iStepX )
    {
        int iOffsetX = 2 * iStepX - iNumSamples + 1;
        [unroll(64)]
        for( int iStepY = 0; iStepY < iNumSamples; ++iStepY )
        {
            int iOffsetY = 2 * iStepY - iNumSamples + 1;
    
            bool4 vbCompares = texShadow.GatherCmp( g_samCompPCF, vSnappedTexCoord / viShadowMapDims, fLightSpaceDepth, int2( iOffsetX, iOffsetY ) );
            
            // Convention for which results go into xyzw - http://msdn.microsoft.com/en-us/library/windows/desktop/hh447088(v=vs.85).aspx
            ShadowTestResults[ 2 * iStepX + 0 ][ 2 * iStepY + 0 ] = vbCompares.w;
            ShadowTestResults[ 2 * iStepX + 1 ][ 2 * iStepY + 0 ] = vbCompares.z;
            ShadowTestResults[ 2 * iStepX + 0 ][ 2 * iStepY + 1 ] = vbCompares.x;
            ShadowTestResults[ 2 * iStepX + 1 ][ 2 * iStepY + 1 ] = vbCompares.y;
        }
    }
    
    float Weights[ MAX_WEIGHTS ];
    CalculateBlurWeights( Weights, g_iBlurKernelSize - 1 );
    Weights[ g_iBlurKernelSize - 1 ] = 0.0f;
    
    // Lerp the weights to account for sub-texel offset
    float2 WeightsLerped[ MAX_WEIGHTS ];
    LerpWeights( Weights, g_iBlurKernelSize, WeightsLerped, vSubTexelCoord );
    
    float fUnshadowedAmount = 0.0f;
    
    // Filter manually using lerped weights
    [unroll(64)]
    for( int x = 0; x < g_iBlurKernelSize; ++x )
    {
        [unroll(64)]
        for( int y = 0; y < g_iBlurKernelSize; ++y )
        {
            fUnshadowedAmount += WeightsLerped[ x ].x * WeightsLerped[ y ].y * ShadowTestResults[ x ][ y ];
        }
    }
    
    return fUnshadowedAmount;
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using variance shadow map
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// For best results, it is desirable for shadow receivers to be rendered into the shadow map. In particular, 
// front faces should be drawn.
//
// To understand why, imagine a case like this:
//
//      Shadow caster,   depth = 0.0    _______________________
//      Shadow receiver, depth = 0.2                    _______x_____
//
//      Average depth          = 0.5  - - - - - - - - - - - - -o
//                                                             |
//                                                             | standard deviation of depth = 0.5
//      Far plane,       depth = 1.0    _______________________V______________________
//
// Case 1: Shadow receiver rendered to shadow map.  As we approach pixel x from the left, depth goes abruptly 
// from 0.0 to 0.2 across a planar boundary.  This is the case where variance gives the exact right answer.
//
// Case 2: Shadow receiver not rendered to shadow map.  As we approach pixel x from the left, depth goes 
// abruptly from 0.0 to 1.0.  This will cause the average depth to be behind the receiver, which violates the 
// assumptions of the technique.
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountVariance( Texture2D<float2> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    float2 vShadowMapMoments = texShadow.Sample( g_samVariance, vShadowTexCoord );
    float fAverage = vShadowMapMoments.x;
    float fVariance = vShadowMapMoments.y - Square( vShadowMapMoments.x );
    float fRemappedDepth = RemappingFunctionVariance( fLightSpaceDepth ).x;

    if( fRemappedDepth < fAverage ) 
    {
        return 1.0f;   // definitely lit
    }
    else
    {
        // Chebyshev's inequality: 
        return fVariance / ( fVariance + Square( fRemappedDepth - fAverage ) );
    }
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using exponential shadow map
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// Unshadowed amount is:
//
//      ( fLightSpaceDepth <= fShadowMapDepth ) ? 1.0f : 0.0f
//
//                      fShadowMapDepth
//      _______________________
//                             |
//                             |
//      fLightSpaceDepth -->   |_________________________
//
// which is approximated by:
//
//      saturate( exp2( k * ( fShadowMapDepth - fLightSpaceDepth ) ) )
//
//                      fShadowMapDepth
//      _______________________
//                             \
//                              \
//      fLightSpaceDepth -->     \_______________________
//
// We rewrite as follows:
//
//      exp2( k * ( fShadowMapDepth - fLightSpaceDepth ) )
//        == fShadowMapDepth_Remapped / fLightSpaceDepth_Remapped
//
// where "Remapped" means f --> exp2( k * f - C )
//
// We choose k and C to make maximum use of the precision afforded by the shadow map format.
//
// For filtering to work properly, it is necessary for shadow receivers to be rendered into the shadow map.
// In particular, front faces should be drawn.
//
// To understand why, imagine a case like this:
//
//                                                       Light |
//                                                             V
//      Shadow caster,   depth = 0.0    _______________________
//
//      Shadow receiver, depth = 0.5                    _______x_____
//
//      Far plane,       depth = 1.0    ______________________________________________
//
// Case 1: Shadow receiver rendered to shadow map.  As we approach pixel x from the left, shadowed amount
// goes smoothly from exp2( k * -0.5f ) ~ 0.0f to exp2( k* 0.0f ) = 1.0f.
//
// Case 2: Shadow receiver not rendered to shadow map.  As we approach pixel x from the left, shadowed amount
// goes from exp2( k * -0.5f ) ~ 0.0f to exp2( k * +0.5f ) ~ +INF, giving a sharp (unfiltered-looking) 
// transition.
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountExponential( Texture2D<float> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    float fShadowMapDepth_Remapped = texShadow.Sample( g_samExponential, vShadowTexCoord );
    return saturate( fShadowMapDepth_Remapped / RemappingFunctionExponential( fLightSpaceDepth ) );
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using exponential variance map
// Returns: 0 --> in full shadow
//          1 --> fully lit
//
// For filtering to work properly, it is necessary for shadow receivers to be rendered into the shadow map.
// In particular, front faces should be drawn.
//
// Exponential variance shadow mapping uses the fact that the variance shadow calculation remains correct if
// depth is replaced by any monotonic function of depth. For instance, the exponential function.
//
// Run two variance tests and take the more occluded result.  One test happens to work better for caster near
// the receiver.  The other test happens to work better for caster far from the receiver.  
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountExponentialVariance( Texture2D<float4> texShadow, float2 vShadowTexCoord, float fLightSpaceDepth )
{
    float4 vShadowMapDepth_ExponentialMoments = texShadow.Sample( g_samExponential, vShadowTexCoord );
    float4 vLightSpaceDepth_Exponential = RemappingFunctionExponentialVariance( fLightSpaceDepth );

    float ShadowMapExponential1stMoment[ 2 ] = { vShadowMapDepth_ExponentialMoments.x, vShadowMapDepth_ExponentialMoments.z };
    float ShadowMapExponential2ndMoment[ 2 ] = { vShadowMapDepth_ExponentialMoments.y, vShadowMapDepth_ExponentialMoments.w } ;
    float LightSpaceExponentialDepth[ 2 ] = { vLightSpaceDepth_Exponential.x, vLightSpaceDepth_Exponential.z };
    float UnshadowedAmount[ 2 ];

    for( uint i = 0; i < 2; ++i )
    {
        float fAverage = ShadowMapExponential1stMoment[ i ];
        float fVariance = ShadowMapExponential2ndMoment[ i ] - Square( ShadowMapExponential1stMoment[ i ] );

        float fSample = LightSpaceExponentialDepth[ i ];

        if( fSample < fAverage ) 
        {
            UnshadowedAmount[ i ] = 1.0f;
        }
        else
        {
            // Chebyshev's inequality: 
            UnshadowedAmount[ i ] = fVariance / ( fVariance + Square( fSample - fAverage ) );
        }
    }

    return min( UnshadowedAmount[ 0 ], UnshadowedAmount[ 1 ] );
}


//-------------------------------------------------------------------------------------------------------------
// Test how much pixel is in shadow
// Returns: 0 --> in full shadow
//          1 --> fully lit
//-------------------------------------------------------------------------------------------------------------
float CalcUnshadowedAmountBase(int iShadow, float3 vPosWorld)
{
    float4x4 mLightViewProj = cbLight.g_LightData[iShadow].m_mLightViewProj;
    Texture2D<RemappedType> texShadow = g_texShadow[ iShadow ]; 

    // Compute pixel position in light space
    float4 vLightSpacePos = mul( float4( vPosWorld, 1.0f ), mLightViewProj ); 
    vLightSpacePos.xyz /= vLightSpacePos.w;
    
    // Translate from surface coords to texture coords
    // Could fold these into the matrix
    vLightSpacePos.xy = ( 0.5f * vLightSpacePos + 0.5f ).xy;
    vLightSpacePos.y = 1.0f - vLightSpacePos.y;
    
    // Depth bias to avoid pixel self-shadowing.  Depth bias is generally unnecessary when using frontface 
    // culling during shadow map generation, because any z-fighting is on the backfaces, where light doesn't reach anyhow.
    vLightSpacePos.z -= cbLight.g_LightData[iShadow].m_fDepthBias;
    
    // Out of bounds
    bool3 vbInBoundsTest = vLightSpacePos.xyz == saturate( vLightSpacePos.xyz );

    // Use the per-shader technique for sampling from the shadow map
    return all( vbInBoundsTest ) ? CalcUnshadowedAmount( texShadow, vLightSpacePos.xy, vLightSpacePos.z ) : 0.0f; 
}


[RootSignature(ROOT_SIG)]
Pixel main(InterpolantsMesh In)
{
    Pixel Out;
    Out.color = cbLight.g_vAmbientColor;
    
#ifdef SUPPORT_NORMAL_MAP
    float3 vNormal = CalcPerPixelNormal(In.texcoord, In.normal, In.tangent);
#else
    float3 vNormal = normalize( In.normal );
#endif

#pragma warning(push)
#pragma warning(disable:3557)   // loop only executes for 1 iteration(s), forcing loop to unroll
    for( uint iLight = 0; iLight < g_iNumLights; ++iLight )
#pragma warning(pop)
    {
        // diffuse
        float fDiffuseIntensity = saturate(-dot(cbLight.g_LightData[iLight].m_vLightWorldDir.xyz, vNormal));

        // specular
        float3 vReflect = reflect(cbLight.g_LightData[iLight].m_vLightWorldDir.xyz, vNormal);
        float3 vEyeToSurface = normalize(In.posworld - cbLight.g_vEye.xyz);
        const float fEpsilon = 1e-10;    // pow( 0.0f, 0.0f ) can explode
        float fSpecularIntensity = pow(saturate(-dot(vReflect, vEyeToSurface)) + fEpsilon, cbLight.g_LightData[iLight].m_fSpecularPower);

        // shadow attenuation
        float fShadowAttenuation = CalcUnshadowedAmountBase( iLight, In.posworld );

        // total
        Out.color += fShadowAttenuation * (fDiffuseIntensity + fSpecularIntensity) * cbLight.g_LightData[iLight].m_vLightColor;
    }
    
#ifdef SUPPORT_NORMAL_MAP
    Out.color *= g_texDiffuse.Sample(g_samNormal, In.texcoord);
#else
    Out.color *= g_texDiffuse.Sample( g_samDiffuse, In.texcoord );
#endif

    return Out;
}


