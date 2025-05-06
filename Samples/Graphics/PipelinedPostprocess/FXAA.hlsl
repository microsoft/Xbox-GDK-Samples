// This software contains source code provided by NVIDIA Corporation.
// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

/*============================================================================
                              COMPILE-IN KNOBS
------------------------------------------------------------------------------
FXAA_EDGE_THRESHOLD - The minimum amount of local contrast required 
                      to apply algorithm.
                      1.0/3.0  - too little
                      1.0/4.0  - good start
                      1.0/8.0  - applies to more edges
                      1.0/16.0 - overkill
------------------------------------------------------------------------------
FXAA_EDGE_THRESHOLD_MIN - Trims the algorithm from processing darks.
                          Perf optimization.
                          1.0/32.0 - visible limit (smaller isn't visible)
                          1.0/16.0 - good compromise
                          1.0/12.0 - upper limit (seeing artifacts)
------------------------------------------------------------------------------
FXAA_SEARCH_STEPS - Maximum number of search steps for end of span.
------------------------------------------------------------------------------
FXAA_SEARCH_THRESHOLD - Controls when to stop searching.
                        1.0/4.0 - seems to be the best quality wise
------------------------------------------------------------------------------
FXAA_SUBPIX_TRIM - Controls sub-pixel aliasing removal.
                   1.0/2.0 - low removal
                   1.0/3.0 - medium removal
                   1.0/4.0 - default removal
                   1.0/8.0 - high removal
                   0.0 - complete removal
------------------------------------------------------------------------------
FXAA_SUBPIX_CAP - Insures fine detail is not completely removed.
                  This is important for the transition of sub-pixel detail,
                  like fences and wires.
                  3.0/4.0 - default (medium amount of filtering)
                  7.0/8.0 - high amount of filtering
                  1.0 - no capping of sub-pixel aliasing removal
============================================================================*/
#define FXAA_EDGE_THRESHOLD      ( 1.0 / 8.0 )
#define FXAA_EDGE_THRESHOLD_MIN  ( 1.0 / 24.0 )
#define FXAA_SEARCH_STEPS        32
#define FXAA_SEARCH_THRESHOLD    ( 1.0 / 4.0 )
#define FXAA_SUBPIX_CAP          ( 3.0 / 4.0 )
#define FXAA_SUBPIX_TRIM         ( 1.0 / 4.0 )

#define FXAA_SUBPIX_TRIM_SCALE ( 1.0 / ( 1.0 - FXAA_SUBPIX_TRIM ) )

#include "Common.hlsli"

// Return the luma, the estimation of luminance from rgb inputs.
// This approximates luma using one FMA instruction,
// skipping normalization and tossing out blue.
// FxaaLuma() will range 0.0 to 2.963210702.
float FxaaLuma( float3 rgb ) 
{
    return rgb.y * ( 0.587/0.299 ) + rgb.x; 
} 

float3 fxaa( float2 pos, SamplerState smpl, Texture2D tex, float2 rcpFrame )
{
    // Early exit if local contrast below edge detect limit
    float3 rgbN = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 0, -1 ) ).xyz;
    float3 rgbW = tex.SampleLevel( smpl, pos.xy, 0.0, int2( -1, 0 ) ).xyz;
    float3 rgbM = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 0, 0 ) ).xyz;
    float3 rgbE = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 1, 0 ) ).xyz;
    float3 rgbS = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 0, 1 ) ).xyz;
    float lumaN = FxaaLuma( rgbN );
    float lumaW = FxaaLuma( rgbW );
    float lumaM = FxaaLuma( rgbM );
    float lumaE = FxaaLuma( rgbE );
    float lumaS = FxaaLuma( rgbS ); 
    float rangeMin = min( lumaM, min( min( lumaN, lumaW ), min( lumaS, lumaE ) ) );
    float rangeMax = max( lumaM, max( max( lumaN, lumaW ), max( lumaS, lumaE ) ) );
    float range = rangeMax - rangeMin;
    float3 rgbL = rgbN + rgbW + rgbM + rgbE + rgbS;
    
    // Compute lowpass
    float lumaL = ( lumaN + lumaW + lumaE + lumaS ) * 0.25;
    float rangeL = abs( lumaL - lumaM );
    float blendL = 0.0;
    if ( range != 0 )
        blendL = max( 0.0, ( rangeL / range ) - FXAA_SUBPIX_TRIM ) * FXAA_SUBPIX_TRIM_SCALE;
    blendL = min( FXAA_SUBPIX_CAP, blendL );
    
    // Choose vertical or horizontal search
    float3 rgbNW = tex.SampleLevel( smpl, pos.xy, 0.0, int2( -1, -1 ) ).xyz;
    float3 rgbNE = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 1, -1 ) ).xyz;
    float3 rgbSW = tex.SampleLevel( smpl, pos.xy, 0.0, int2( -1, 1 ) ).xyz;
    float3 rgbSE = tex.SampleLevel( smpl, pos.xy, 0.0, int2( 1, 1 ) ).xyz;
    rgbL += ( rgbNW + rgbNE + rgbSW + rgbSE );
    rgbL *= 1.0 / 9.0;
    float lumaNW = FxaaLuma( rgbNW );
    float lumaNE = FxaaLuma( rgbNE );
    float lumaSW = FxaaLuma( rgbSW );
    float lumaSE = FxaaLuma( rgbSE );
    float edgeVert = abs( ( 0.25 * lumaNW ) + ( -0.5 * lumaN ) + ( 0.25 * lumaNE ) ) +
                     abs( ( 0.50 * lumaW  ) + ( -1.0 * lumaM ) + ( 0.50 * lumaE  ) ) +
                     abs( ( 0.25 * lumaSW ) + ( -0.5 * lumaS ) + ( 0.25 * lumaSE ) );
    float edgeHorz = abs( ( 0.25 * lumaNW ) + ( -0.5 * lumaW ) + ( 0.25 * lumaSW ) ) +
                     abs( ( 0.50 * lumaN  ) + ( -1.0 * lumaM ) + ( 0.50 * lumaS  ) ) +
                     abs( ( 0.25 * lumaNE ) + ( -0.5 * lumaE ) + ( 0.25 * lumaSE ) );
    bool horzSpan = edgeHorz >= edgeVert;
    float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
    if( !horzSpan )
    {
        lumaN = lumaW;
        lumaS = lumaE;
    }
    float gradientN = abs( lumaN - lumaM );
    float gradientS = abs( lumaS - lumaM );
    lumaN = ( lumaN + lumaM ) * 0.5;
    lumaS = ( lumaS + lumaM ) * 0.5;
    
    // Choose side of pixel where gradient is highest
    bool pairN = gradientN >= gradientS;
    if( !pairN )
    {
        lumaN = lumaS;
        gradientN = gradientS;
        lengthSign *= -1.0;
    }
    float2 posN;
    posN.x = pos.x + ( horzSpan ? 0.0 : lengthSign * 0.5 );
    posN.y = pos.y + ( horzSpan ? lengthSign * 0.5 : 0.0 );
    
    gradientN *= FXAA_SEARCH_THRESHOLD;
    
    float2 posP = posN;
    float2 offNP = horzSpan ? float2( rcpFrame.x, 0.0 ) : float2( 0.0f, rcpFrame.y ); 
    float lumaEndN = lumaN;
    float lumaEndP = lumaN;
    bool doneN = false;
    bool doneP = false;
    posN += offNP * float2( -1.0, -1.0 );
    posP += offNP * float2( 1.0,  1.0 );

    for( int i = 0; i < FXAA_SEARCH_STEPS; i++ ) 
    {
        if( !doneN ) 
            lumaEndN = FxaaLuma( tex.SampleLevel( smpl, posN.xy, 0 ).xyz );
        if( !doneP )
            lumaEndP = FxaaLuma( tex.SampleLevel( smpl, posP.xy, 0 ).xyz );
       
        doneN = doneN || ( abs( lumaEndN - lumaN ) >= gradientN );
        doneP = doneP || ( abs( lumaEndP - lumaN ) >= gradientN );
        if( doneN && doneP ) 
            break;
        if( !doneN )
            posN -= offNP;
        if( !doneP )
            posP += offNP; 
    }

    // Handle if center is on positive or negative side
    float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
    float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;
    bool directionN = dstN < dstP;
    lumaEndN = directionN ? lumaEndN : lumaEndP;
    
    // Check if pixel is in section of span which gets no filtering
    if( ( ( lumaM - lumaN ) < 0.0) == ( ( lumaEndN - lumaN ) < 0.0 ) ) 
        lengthSign = 0.0;
 
    // Compute sub-pixel offset and filter span
    float spanLength = ( dstP + dstN );
    dstN = directionN ? dstN : dstP;
    float subPixelOffset = ( 0.5 + ( dstN * ( -1.0 / spanLength ) ) ) * lengthSign;
    float2 posF = float2( pos.x + ( horzSpan ? 0.0 : subPixelOffset ), pos.y + ( horzSpan ? subPixelOffset : 0.0 ) );
    float3 rgbF = tex.SampleLevel( smpl, posF, 0 ).xyz;
    return lerp( rgbF, rgbL, blendL ); 
}

struct FXAAConstantBuffer
{
    float2      fxaaPixelSize;
    float       width;
    float       height;
};
ConstantBuffer<FXAAConstantBuffer> cb1 : register(b0);

// SRV texture (previous frame)
Texture2D<float4> Texture : register(t0);

// Sampler
SamplerState LinearSampler : register(s0);

// UAV texture (output)
RWTexture2D<float4> OutTex : register(u0);

[RootSignature(ROOT_SIGNATURE_COMPUTE)]
[numthreads(8, 8, 1)]
void main(int3 threadId : SV_DispatchThreadID)
{
    float texcoordx = threadId.x / cb1.width;
    float texcoordy = threadId.y / cb1.height;
    OutTex[threadId.xy] =  float4(fxaa(float2(texcoordx, texcoordy), LinearSampler, Texture, cb1.fxaaPixelSize), 1.0f);
}
