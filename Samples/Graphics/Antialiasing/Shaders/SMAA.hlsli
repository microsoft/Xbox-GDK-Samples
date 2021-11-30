/**
This file is based on or incorporates material from the projects listed below (collectively, ?Third Party Code?). 
Microsoft is not the original author of the Third Party Code. The original copyright notice and the license under 
which Microsoft received such Third Party Code, are set forth below. Such licenses and notices are provided for 
informational purposes only. Microsoft, not the third party, licenses the Third Party Code to you under the terms 
set forth in the EULA for the Microsoft Product. Microsoft reserves all rights not expressly granted under this agreement, 
whether by implication, estoppel or otherwise. 

iryoku-smaa (http://www.iryoku.com/smaa/)

Copyright (C) 2011 Jorge Jimenez (jorge@iryoku.com)
Copyright (C) 2011 Belen Masia (bmasia@unizar.es) 
Copyright (C) 2011 Jose I. Echevarria (joseignacioechevarria@gmail.com) 
Copyright (C) 2011 Fernando Navarro (fernandn@microsoft.com) 
Copyright (C) 2011 Diego Gutierrez (diegog@unizar.es)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the following disclaimer
      in the documentation and/or other materials provided with the 
      distribution:

      "Uses SMAA. Copyright (C) 2011 by Jorge Jimenez, Jose I. Echevarria,
       Belen Masia, Fernando Navarro and Diego Gutierrez."

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS 
IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are 
those of the authors and should not be interpreted as representing official
policies, either expressed or implied, of the copyright holders.

Provided for Informational Purposes Only
BSD License

All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ""AS IS"" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* Please refer http://www.iryoku.com/smaa/ for more information regarding this algorithm */

#define SMAA_PRESET_HIGH 1

#define SMAA_FORCE_DIAGONAL_DETECTION 1
#define SMAA_FORCE_CORNER_DETECTION 1

#if SMAA_PRESET_LOW == 1
#define SMAA_THRESHOLD 0.15
#define SMAA_MAX_SEARCH_STEPS 4
#define SMAA_MAX_SEARCH_STEPS_DIAG 0
#define SMAA_CORNER_ROUNDING 100
#elif SMAA_PRESET_MEDIUM == 1
#define SMAA_THRESHOLD 0.1
#define SMAA_MAX_SEARCH_STEPS 8
#define SMAA_MAX_SEARCH_STEPS_DIAG 0
#define SMAA_CORNER_ROUNDING 100
#elif SMAA_PRESET_HIGH == 1
#define SMAA_THRESHOLD 0.03
#define SMAA_MAX_SEARCH_STEPS 16
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25
#elif SMAA_PRESET_ULTRA == 1
#define SMAA_THRESHOLD 0.05
#define SMAA_MAX_SEARCH_STEPS 32
#define SMAA_MAX_SEARCH_STEPS_DIAG 16
#define SMAA_CORNER_ROUNDING 25
#endif

//-----------------------------------------------------------------------------
// Configurable Defines

/**
 * SMAA_THRESHOLD specifies the threshold or sensitivity to edges.
 * Lowering this value you will be able to detect more edges at the expense of
 * performance. 
 *
 * Range: [0, 0.5]
 *   0.1 is a reasonable value, and allows to catch most visible edges.
 *   0.05 is a rather overkill value, that allows to catch 'em all.
 *
 *   If temporal supersampling is used, 0.2 could be a reasonable value, as low
 *   contrast edges are properly filtered by just 2x.
 */
#ifndef SMAA_THRESHOLD
#define SMAA_THRESHOLD 0.2
#endif

/**
 * SMAA_DEPTH_THRESHOLD specifies the threshold for depth edge detection.
 * 
 * Range: depends on the depth range of the scene.
 */
#ifndef SMAA_DEPTH_THRESHOLD
#define SMAA_DEPTH_THRESHOLD (0.1 * SMAA_THRESHOLD)
#endif

/**
 * SMAA_MAX_SEARCH_STEPS specifies the maximum steps performed in the
 * horizontal/vertical pattern searches, at each side of the pixel.
 *
 * In number of pixels, it's actually the double. So the maximum line length
 * perfectly handled by, for example 16, is 64 (by perfectly, we meant that
 * longer lines won't look as good, but still antialiased).
 *
 * Range: [0, 98]
 */
#ifndef SMAA_MAX_SEARCH_STEPS
#define SMAA_MAX_SEARCH_STEPS 16
#endif

/**
 * SMAA_MAX_SEARCH_STEPS_DIAG specifies the maximum steps performed in the
 * diagonal pattern searches, at each side of the pixel. In this case we jump
 * one pixel at time, instead of two.
 *
 * Range: [0, 20]; set it to 0 to disable diagonal processing.
 *
 * On high-end machines it is cheap (between a 0.8x and 0.9x slower for 16 
 * steps), but it can have a significant impact on older machines.
 */
#ifndef SMAA_MAX_SEARCH_STEPS_DIAG
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#endif

/**
 * SMAA_CORNER_ROUNDING specifies how much sharp corners will be rounded.
 *
 * Range: [0, 100]; set it to 100 to disable corner detection.
 */
#ifndef SMAA_CORNER_ROUNDING
#define SMAA_CORNER_ROUNDING 25
#endif

/**
 * Temporal reprojection allows to remove ghosting artifacts when using
 * temporal supersampling. We use the CryEngine 3 method which also introduces
 * velocity weighting. This feature is of extreme importance for totally
 * removing ghosting. More information here:
 *    http://iryoku.com/aacourse/downloads/13-Anti-Aliasing-Methods-in-CryENGINE-3.pdf
 *
 * Note that you'll need to setup a velocity buffer for enabling reprojection.
 * For static geometry, saving the previous depth buffer is a viable
 * alternative.
 */
#ifndef SMAA_REPROJECTION
#define SMAA_REPROJECTION 0
#endif

/**
 * SMAA_REPROJECTION_WEIGHT_SCALE controls the velocity weighting. It allows to
 * remove ghosting trails behind the moving object, which are not removed by
 * just using reprojection. Using low values will exhibit ghosting, while using
 * high values will disable temporal supersampling under motion.
 *
 * Behind the scenes, velocity weighting removes temporal supersampling when
 * the velocity of the subsamples differs (meaning they are different objects).
 *
 * Range: [0, 80]
 */
#define SMAA_REPROJECTION_WEIGHT_SCALE 30.0

//-----------------------------------------------------------------------------
// Non-Configurable Defines

#ifndef SMAA_AREATEX_MAX_DISTANCE
#define SMAA_AREATEX_MAX_DISTANCE 16
#endif
#ifndef SMAA_AREATEX_MAX_DISTANCE_DIAG
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20
#endif
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / float2(160.0, 560.0 ) )
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)

SamplerState linearSampler : register(s1);
#define SMAASampleLevelZero( tex, coord ) tex.SampleLevel( linearSampler, coord, 0 )
#define SMAASampleLevelZeroOffset( tex, coord, offset) tex.SampleLevel( linearSampler, coord, 0, offset )

cbuffer SMAAConstantBuffers : register( b0 )
{
    float4 cb_subsampleIndices;
    float2 cb_smaaPixelSize;
};

struct VSSmaaEdgeIn
{
    float4 pos : POSITION;
    float2 tex : TEXCOORD;
};

struct VSSmaaEdgeOut
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float4 offset[3] : OFFSET;
};

//-----------------------------------------------------------------------------
// Vertex Shaders

// Edge Detection Vertex Shader
VSSmaaEdgeOut SMAAEdgeDetectionVS( VSSmaaEdgeIn vsIn )
{
    VSSmaaEdgeOut vsOut;
    
    vsOut.pos = vsIn.pos;
    vsOut.tex = vsIn.tex;

    vsOut.offset[0] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
    vsOut.offset[1] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
    vsOut.offset[2] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4(-2.0, 0.0, 0.0, -2.0);

    return vsOut;
}

struct VSSmaaBlendingWeightIn
{
    float4 pos : POSITION;
    float2 tex : TEXCOORD;
};

struct VSSmaaBlendingWeightOut
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float2 pixcoord : PIXCOORD;
    float4 offset[3] : OFFSET;
};

//  Blend Weight Calculation Vertex Shader
VSSmaaBlendingWeightOut SMAABlendingWeightCalculationVS( VSSmaaBlendingWeightIn vsIn ) 
{
    VSSmaaBlendingWeightOut vsOut;
                                         
    vsOut.pos = vsIn.pos;
    vsOut.tex = vsIn.tex;

    vsOut.pixcoord = vsIn.tex / cb_smaaPixelSize;

    // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
    vsOut.offset[0] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4( -0.25, -0.125,  1.25, -0.125 );
    vsOut.offset[1] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4( -0.125, -0.25, -0.125,  1.25 );

    // And these for the searches, they indicate the ends of the loops:
    vsOut.offset[2] = float4( vsOut.offset[0].xz, vsOut.offset[1].yw ) + 
                      float4( -2.0, 2.0, -2.0, 2.0 ) *
                      cb_smaaPixelSize.xxyy * float( SMAA_MAX_SEARCH_STEPS );

    return vsOut;
}

struct VSSmaaNeighborhoodBlendingIn
{
    float4 pos : POSITION;
    float2 tex : TEXCOORD;
};

struct VSSmaaNeighborhoodBlendingOut
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
    float4 offset[2] : OFFSET;
};

// Neighborhood Blending Vertex Shader
VSSmaaNeighborhoodBlendingOut SMAANeighborhoodBlendingVS( VSSmaaNeighborhoodBlendingIn vsIn ) 
{
    VSSmaaNeighborhoodBlendingOut vsOut;
    
    vsOut.pos = vsIn.pos;
    vsOut.tex = vsIn.tex;

    vsOut.offset[0] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4( -1.0, 0.0, 0.0, -1.0 );
    vsOut.offset[1] = vsIn.tex.xyxy + cb_smaaPixelSize.xyxy * float4( 1.0, 0.0, 0.0,  1.0 );

    return vsOut;
}

// Pixel Shaders

Texture2D colorTex : register( t0 );
Texture2D depthTex : register( t1 );

// Edge Detection Pixel Shaders (First Pass)

/**
 * Luma Edge Detection
 *
 * IMPORTANT NOTICE: luma edge detection requires gamma-corrected colors, and
 * thus 'colorTex' should be a non-sRGB texture.
 */
float4 SMAALumaEdgeDetectionPS( VSSmaaEdgeOut psIn ) : SV_TARGET
{
    // Calculate the threshold:
    float2 threshold = float2( SMAA_THRESHOLD, SMAA_THRESHOLD );

    // Calculate lumas:
    float3 weights = float3( 0.2126, 0.7152, 0.0722 );
    float L = dot( SMAASampleLevelZero( colorTex, psIn.tex ).rgb, weights);
    float Lleft = dot( SMAASampleLevelZero( colorTex, psIn.offset[0].xy ).rgb, weights );
    float Ltop  = dot( SMAASampleLevelZero( colorTex, psIn.offset[0].zw ).rgb, weights );

    // We do the usual threshold:
    float4 delta;
    delta.xy = abs( L - float2( Lleft, Ltop ) );
    float2 edges = step( threshold, delta.xy );

    // Then discard if there is no edge:
    if ( dot( edges, float2( 1.0, 1.0 ) ) == 0.0 )
        discard;

    // Calculate right and bottom deltas:
    float Lright = dot( SMAASampleLevelZero( colorTex, psIn.offset[1].xy ).rgb, weights );
    float Lbottom  = dot( SMAASampleLevelZero( colorTex, psIn.offset[1].zw ).rgb, weights );
    delta.zw = abs( L - float2( Lright, Lbottom ) );

    // Calculate the maximum delta in the direct neighborhood:
    float2 maxDelta = max( delta.xy, delta.zw );
    maxDelta = max( maxDelta.xx, maxDelta.yy );

    // Calculate left-left and top-top deltas:
    float Lleftleft = dot( SMAASampleLevelZero( colorTex, psIn.offset[2].xy ).rgb, weights );
    float Ltoptop = dot( SMAASampleLevelZero( colorTex, psIn.offset[2].zw ).rgb, weights );
    delta.zw = abs( float2( Lleft, Ltop ) - float2( Lleftleft, Ltoptop ) );

    // Calculate the final maximum delta:
    maxDelta = max( maxDelta.xy, delta.zw );

    /**
     * Each edge with a delta in luma of less than 50% of the maximum luma
     * surrounding this pixel is discarded. This allows to eliminate spurious
     * crossing edges, and is based on the fact that, if there is too much
     * contrast in a direction, that will hide contrast in the other
     * neighbors.
     * This is done after the discard intentionally as this situation doesn't
     * happen too frequently (but it's important to do as it prevents some 
     * edges from going undetected).
     */
    edges.xy *= step( 0.5 * maxDelta, delta.xy );

    return float4( edges, 0.0, 0.0 );
}

/**
 * Color Edge Detection
 *
 * IMPORTANT NOTICE: color edge detection requires gamma-corrected colors, and
 * thus 'colorTex' should be a non-sRGB texture.
 */
float4 SMAAColorEdgeDetectionPS( VSSmaaEdgeOut psIn ) : SV_TARGET
{
    // Calculate the threshold:
    float2 threshold = float2( SMAA_THRESHOLD, SMAA_THRESHOLD );
    
    // Calculate color deltas:
    float4 delta;
    float3 C = SMAASampleLevelZero( colorTex, psIn.tex ).rgb;

    float3 Cleft = SMAASampleLevelZero( colorTex, psIn.offset[0].xy ).rgb;
    float3 t = abs( C - Cleft );
    delta.x = max( max( t.r, t.g ), t.b );

    float3 Ctop  = SMAASampleLevelZero( colorTex, psIn.offset[0].zw ).rgb;
    t = abs( C - Ctop );
    delta.y = max( max( t.r, t.g ), t.b );

    // We do the usual threshold:
    float2 edges = step( threshold, delta.xy );

    // Then discard if there is no edge:
    if ( dot( edges, float2( 1.0, 1.0 ) ) == 0.0 )
        discard;

    // Calculate right and bottom deltas:
    float3 Cright = SMAASampleLevelZero( colorTex, psIn.offset[1].xy ).rgb;
    t = abs( C - Cright );
    delta.z = max( max( t.r, t.g ), t.b );

    float3 Cbottom  = SMAASampleLevelZero( colorTex, psIn.offset[1].zw ).rgb;
    t = abs( C - Cbottom );
    delta.w = max( max( t.r, t.g ), t.b );

    // Calculate the maximum delta in the direct neighborhood:
    float maxDelta = max( max( max( delta.x, delta.y ), delta.z ), delta.w );

    // Calculate left-left and top-top deltas:
    float3 Cleftleft  = SMAASampleLevelZero( colorTex, psIn.offset[2].xy ).rgb;
    t = abs( C - Cleftleft );
    delta.z = max( max( t.r, t.g ), t.b );

    float3 Ctoptop = SMAASampleLevelZero( colorTex, psIn.offset[2].zw ).rgb;
    t = abs( C - Ctoptop );
    delta.w = max( max( t.r, t.g ), t.b );

    // Calculate the final maximum delta:
    maxDelta = max( max( maxDelta, delta.z ), delta.w );

    // Local contrast adaptation in action:
    edges.xy *= step( 0.5 * maxDelta, delta.xy );

    return float4( edges, 0.0, 0.0 );
}

float3 LinearizeDepth(float3 zoverw)
{
    float near = 0.05;
    float far = 1000.0;
    float3 nearPlane = float3( near, near, near );
    float3 farPlane = float3( far, far, far );	
    return( 2.0 * nearPlane ) / ( farPlane + nearPlane - zoverw * ( farPlane - nearPlane ) );	
}

// Depth Edge Detection
float4 SMAADepthEdgeDetectionPS( VSSmaaEdgeOut psIn ) : SV_TARGET
{
    float3 neighbours = depthTex.Gather( linearSampler, psIn.tex + cb_smaaPixelSize * float2( -0.5, -0.5 ), 0 ).grb;
    neighbours = LinearizeDepth( neighbours );
    float2 delta = abs( neighbours.xx - float2( neighbours.y, neighbours.z ) );
    float2 edges = step( SMAA_DEPTH_THRESHOLD, delta );

    if ( dot( edges, float2( 1.0, 1.0 ) ) == 0.0 )
        discard;

    return float4( edges, 0.0, 0.0 );
}


Texture2D edgesTex  : register( t0 );
Texture2D areaTex   : register( t1 );
Texture2D searchTex : register( t2 );
SamplerState pointSampler : register( s0 );
#define SMAASampleLevelZeroPoint( tex, coord ) tex.SampleLevel( pointSampler, coord, 0 )
#define SMAASamplePoint( tex, coord ) SMAASampleLevelZeroPoint( tex, coord )

// Diagonal Search Functions
// These functions allows to perform diagonal pattern searches.

float SMAASearchDiag1( float2 texcoord, float2 dir, float c ) 
{
    texcoord += dir * cb_smaaPixelSize;
    float2 e = float2( 0.0, 0.0 );
    float i;
    for ( i = 0.0; i < float( SMAA_MAX_SEARCH_STEPS_DIAG ); i++ ) 
    {
        e.rg = SMAASampleLevelZero( edgesTex, texcoord ).rg;
        [flatten] if ( dot( e, float2( 1.0, 1.0 ) ) < 1.9 ) break;
        texcoord += dir * cb_smaaPixelSize;
    }
    return i + float( e.g > 0.9 ) * c;
}

float SMAASearchDiag2( float2 texcoord, float2 dir, float c ) 
{
    texcoord += dir * cb_smaaPixelSize;
    float2 e = float2( 0.0, 0.0 );
    float i;
    for ( i = 0.0; i < float( SMAA_MAX_SEARCH_STEPS_DIAG ); i++ ) 
    {
        e.g = SMAASampleLevelZero( edgesTex, texcoord).g;
        e.r = SMAASampleLevelZeroOffset( edgesTex, texcoord, int2( 1, 0 ) ).r;
        [flatten] if ( dot( e, float2( 1.0, 1.0 ) ) < 1.9 ) break;
        texcoord += dir * cb_smaaPixelSize;
    }
    return i + float( e.g > 0.9 ) * c;
}

// Similar to SMAAArea, this calculates the area corresponding to a certain
// diagonal distance and crossing edges 'e'.
float2 SMAAAreaDiag( float2 dist, float2 e, float offset ) {
    float2 texcoord = float( SMAA_AREATEX_MAX_DISTANCE_DIAG ) * e + dist;

    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + ( 0.5 * SMAA_AREATEX_PIXEL_SIZE );

    // Diagonal areas are on the second half of the texture:
    texcoord.x += 0.5;

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return SMAASampleLevelZero( areaTex, texcoord ).rg;
}

// This searches for diagonal patterns and returns the corresponding weights.
float2 SMAACalculateDiagWeights( float2 texcoord, float2 e ) {
    float2 weights = float2( 0.0, 0.0 );

    float2 d;
    d.x = e.r > 0.0? SMAASearchDiag1( texcoord, float2( -1.0,  1.0 ), 1.0 ) : 0.0;
    d.y = SMAASearchDiag1( texcoord, float2( 1.0, -1.0 ), 0.0 );

    [branch]
    if ( d.r + d.g > 2.0 )
    { // d.r + d.g + 1 > 3
        float4 coords = mad( float4( -d.r, d.r, d.g, -d.g ), cb_smaaPixelSize.xyxy, texcoord.xyxy );

        float4 c;
        c.x = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2( -1,  0 ) ).g;
        c.y = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2(  0,  0 ) ).r;
        c.z = SMAASampleLevelZeroOffset( edgesTex, coords.zw, int2(  1,  0 ) ).g;
        c.w = SMAASampleLevelZeroOffset( edgesTex, coords.zw, int2(  1, -1 ) ).r;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float( SMAA_MAX_SEARCH_STEPS_DIAG ) - 1.0;
        e *= step( d.rg, float2( t, t ) );

        weights += SMAAAreaDiag( d, e, float( cb_subsampleIndices.z ) );
    }

    d.x = SMAASearchDiag2( texcoord, float2( -1.0, -1.0 ), 0.0 );
    float right = SMAASampleLevelZeroOffset( edgesTex, texcoord, int2( 1, 0 ) ).r;
    d.y = right > 0.0? SMAASearchDiag2( texcoord, float2( 1.0, 1.0 ), 1.0 ) : 0.0;

    [branch]
    if ( d.r + d.g > 2.0 ) 
    { // d.r + d.g + 1 > 3
        float4 coords = mad( float4( -d.r, -d.r, d.g, d.g ), cb_smaaPixelSize.xyxy, texcoord.xyxy );

        float4 c;
        c.x  = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2( -1,  0 ) ).g;
        c.y  = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2(  0, -1 ) ).r;
        c.zw = SMAASampleLevelZeroOffset( edgesTex, coords.zw, int2(  1,  0 ) ).gr;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float( SMAA_MAX_SEARCH_STEPS_DIAG ) - 1.0;
        e *= step( d.rg, float2( t, t ) );

        weights += SMAAAreaDiag( d, e, float( cb_subsampleIndices.w ) ).gr;
    }

    return weights;
}

//-----------------------------------------------------------------------------
// Horizontal/Vertical Search Functions

float SMAASearchLength( float2 e, float bias, float scale ) {
    e.r = bias + e.r * scale;
    return 255.0 * SMAASampleLevelZeroPoint( searchTex, e ).r;
}

// Horizontal/vertical search functions for the 2nd pass.
float SMAASearchXLeft( float2 texcoord, float end )
{
    float2 e = float2( 0.0, 1.0 );
    while ( texcoord.x > end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0 )
    { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero( edgesTex, texcoord ).rg;
        texcoord -= float2( 2.0, 0.0 ) * cb_smaaPixelSize;
    }

    // We correct the previous (-0.25, -0.125) offset we applied:
    texcoord.x += 0.25 * cb_smaaPixelSize.x;

    // The searches are bias by 1, so adjust the coords accordingly:
    texcoord.x += cb_smaaPixelSize.x;

    // Disambiguate the length added by the last step:
    texcoord.x += 2.0 * cb_smaaPixelSize.x; // Undo last step
    texcoord.x -= cb_smaaPixelSize.x * SMAASearchLength( e, 0.0, 0.5 );

    return texcoord.x;
}

float SMAASearchXRight( float2 texcoord, float end ) {
    float2 e = float2( 0.0, 1.0 );
    while ( texcoord.x < end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0 )
    { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero( edgesTex, texcoord ).rg;
        texcoord += float2( 2.0, 0.0 ) * cb_smaaPixelSize;
    }

    texcoord.x -= 0.25 * cb_smaaPixelSize.x;
    texcoord.x -= cb_smaaPixelSize.x;
    texcoord.x -= 2.0 * cb_smaaPixelSize.x;
    texcoord.x += cb_smaaPixelSize.x * SMAASearchLength( e, 0.5, 0.5 );
    return texcoord.x;
}

float SMAASearchYUp( float2 texcoord, float end ) {
    float2 e = float2( 1.0, 0.0 );
    while ( texcoord.y > end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0 )
    { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero( edgesTex, texcoord ).rg;
        texcoord -= float2( 0.0, 2.0 ) * cb_smaaPixelSize;
    }

    texcoord.y += 0.25 * cb_smaaPixelSize.y;
    texcoord.y += cb_smaaPixelSize.y;
    texcoord.y += 2.0 * cb_smaaPixelSize.y;
    texcoord.y -= cb_smaaPixelSize.y * SMAASearchLength( e.gr, 0.0, 0.5 );
    return texcoord.y;
}

float SMAASearchYDown( float2 texcoord, float end ) {
    float2 e = float2( 1.0, 0.0 );
    while ( texcoord.y < end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero( edgesTex, texcoord ).rg;
        texcoord += float2( 0.0, 2.0 ) * cb_smaaPixelSize;
    }
    
    texcoord.y -= 0.25 * cb_smaaPixelSize.y;
    texcoord.y -= cb_smaaPixelSize.y;
    texcoord.y -= 2.0 * cb_smaaPixelSize.y;
    texcoord.y += cb_smaaPixelSize.y * SMAASearchLength( e.gr, 0.5, 0.5 );
    return texcoord.y;
}

// Ok, we have the distance and both crossing edges. So, what are the areas
 // at each side of current edge?
float2 SMAAArea( float2 dist, float e1, float e2, float offset ) {
    // Rounding prevents precision errors of bilinear filtering:
    float2 texcoord = float( SMAA_AREATEX_MAX_DISTANCE ) * round( 4.0 * float2( e1, e2 ) ) + dist;
    
    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + ( 0.5 * SMAA_AREATEX_PIXEL_SIZE );

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return SMAASampleLevelZero( areaTex, texcoord ).rg;
}

//-----------------------------------------------------------------------------
// Corner Detection Functions

void SMAADetectHorizontalCornerPattern( inout float2 weights, float2 texcoord, float2 d )
{
    #if SMAA_CORNER_ROUNDING < 100 || SMAA_FORCE_CORNER_DETECTION == 1
    float4 coords = mad( float4( d.x, 0.0, d.y, 0.0 ), cb_smaaPixelSize.xyxy, texcoord.xyxy );
    float2 e;
    e.r = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2( 0.0,  1.0 ) ).r;
    bool left = abs( d.x ) < abs( d.y );
    e.g = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2( 0.0, -2.0 ) ).r;
    if ( left )
        weights *= saturate( float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e );

    e.r = SMAASampleLevelZeroOffset( edgesTex, coords.zw, int2( 1.0,  1.0 ) ).r;
    e.g = SMAASampleLevelZeroOffset( edgesTex, coords.zw, int2( 1.0, -2.0 ) ).r;
    if ( !left )
        weights *= saturate( float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e );
    #endif
}

void SMAADetectVerticalCornerPattern( inout float2 weights, float2 texcoord, float2 d )
{
    #if SMAA_CORNER_ROUNDING < 100 || SMAA_FORCE_CORNER_DETECTION == 1
    float4 coords = mad( float4( 0.0, d.x, 0.0, d.y ), cb_smaaPixelSize.xyxy, texcoord.xyxy );
    float2 e;
    e.r = SMAASampleLevelZeroOffset( edgesTex, coords.xy, int2( 1.0, 0.0 ) ).g;
    bool left = abs( d.x ) < abs( d.y );
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2( -2.0, 0.0 ) ).g;
    if ( left )
        weights *= saturate(float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e );

    e.r = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2(  1.0, 1.0 ) ).g;
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2( -2.0, 1.0 ) ).g;
    if (!left)
        weights *= saturate(float(SMAA_CORNER_ROUNDING ) / 100.0 + 1.0 - e );
    #endif
}


//-----------------------------------------------------------------------------
// Blending Weight Calculation Pixel Shader (Second Pass)

float4 SMAABlendingWeightCalculationPS( VSSmaaBlendingWeightOut psIn ) : SV_TARGET
{ 
    float4 weights = float4( 0.0, 0.0, 0.0, 0.0 );
    float2 e = SMAASampleLevelZero(edgesTex, psIn.tex).rg;

    [branch]
    if ( e.g > 0.0 ) { // Edge at north
        #if SMAA_MAX_SEARCH_STEPS_DIAG > 0 || SMAA_FORCE_DIAGONAL_DETECTION == 1
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        weights.rg = SMAACalculateDiagWeights( psIn.tex, e );

        // We give priority to diagonals, so if we find a diagonal we skip 
        // horizontal/vertical processing.
        [branch]
        if ( dot( weights.rg, float2( 1.0, 1.0 ) ) == 0.0 ) {
        #endif

        float2 d;

        // Find the distance to the left:
        float2 coords;
        coords.x = SMAASearchXLeft( psIn.offset[0].xy, psIn.offset[2].x );
        coords.y = psIn.offset[1].y; // offset[1].y = texcoord.y - 0.25 * cb_smaaPixelSize.y (@CROSSING_OFFSET)
        d.x = coords.x;

        // Now fetch the left crossing edges, two at a time using bilinear
        // filtering. Sampling at -0.25 (see @CROSSING_OFFSET) enables to
        // discern what value each edge has:
        float e1 = SMAASampleLevelZero( edgesTex, coords ).r;

        // Find the distance to the right:
        coords.x = SMAASearchXRight( psIn.offset[0].zw, psIn.offset[2].y );
        d.y = coords.x;

        // We want the distances to be in pixel units (doing this here allow to
        // better interleave arithmetic and memory accesses):
        d = d / cb_smaaPixelSize.x - psIn.pixcoord.x;

        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        float2 sqrt_d = sqrt( abs( d ) );

        // Fetch the right crossing edges:
        float e2 = SMAASampleLevelZeroOffset( edgesTex, coords, int2(1, 0 ) ).r;

        // Ok, we know how this pattern looks like, now it is time for getting
        // the actual area:
        weights.rg = SMAAArea( sqrt_d, e1, e2, float( cb_subsampleIndices.y ) );

        // Fix corners:
        SMAADetectHorizontalCornerPattern( weights.rg, psIn.tex, d );

        #if SMAA_MAX_SEARCH_STEPS_DIAG > 0 || SMAA_FORCE_DIAGONAL_DETECTION == 1
        } else
            e.r = 0.0; // Skip vertical processing.
        #endif
    }

    [branch]
    if ( e.r > 0.0 ) { // Edge at west
        float2 d;

        // Find the distance to the top:
        float2 coords;
        coords.y = SMAASearchYUp( psIn.offset[1].xy, psIn.offset[2].z );
        coords.x = psIn.offset[0].x; // offset[1].x = texcoord.x - 0.25 * cb_smaaPixelSize.x;
        d.x = coords.y;

        // Fetch the top crossing edges:
        float e1 = SMAASampleLevelZero(edgesTex, coords).g;

        // Find the distance to the bottom:
        coords.y = SMAASearchYDown( psIn.offset[1].zw, psIn.offset[2].w);
        d.y = coords.y;

        // We want the distances to be in pixel units:
        d = d / cb_smaaPixelSize.y - psIn.pixcoord.y;

        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        float2 sqrt_d = sqrt( abs( d ) );

        // Fetch the bottom crossing edges:
        float e2 = SMAASampleLevelZeroOffset( edgesTex, coords, int2( 0, 1 ) ).g;

        // Get the area for this direction:
        weights.ba = SMAAArea( sqrt_d, e1, e2, float( cb_subsampleIndices.x ) );

        // Fix corners:
        SMAADetectVerticalCornerPattern( weights.ba, psIn.tex, d );
    }

    return weights;
}

Texture2D blendTex : register( t1 );

//-----------------------------------------------------------------------------
// Neighborhood Blending Pixel Shader (Third Pass)

float4 SMAANeighborhoodBlendingPS( VSSmaaNeighborhoodBlendingOut psIn ) : SV_TARGET
{
    // Fetch the blending weights for current pixel:
    float4 a;
    a.xz = SMAASampleLevelZero( blendTex, psIn.tex ).xz;
    a.y = SMAASampleLevelZero( blendTex, psIn.offset[1].zw ).g;
    a.w = SMAASampleLevelZero( blendTex, psIn.offset[1].xy ).a;

    // Is there any blending weight with a value greater than 0.0?
    [branch]
    if ( dot( a, float4(1.0, 1.0, 1.0, 1.0 ) ) < 1e-5 )
        return SMAASampleLevelZero( colorTex, psIn.tex );
    else {
        float4 color = float4( 0.0, 0.0, 0.0, 0.0 );

        // Up to 4 lines can be crossing a pixel (one through each edge). We
        // favor blending by choosing the line with the maximum weight for each
        // direction:
        float2 offset;
        offset.x = a.a > a.b? a.a : -a.b; // left vs. right 
        offset.y = a.g > a.r? a.g : -a.r; // top vs. bottom

        // Then we go in the direction that has the maximum weight:
        if ( abs( offset.x ) > abs( offset.y ) ) // horizontal vs. vertical
            offset.y = 0.0;
        else
            offset.x = 0.0;

        #if SMAA_REPROJECTION == 1
        // Fetch the opposite color and lerp by hand:
        float4 C = SMAASampleLevelZero( colorTex, psIn.tex );
        texcoord += sign( offset ) * cb_smaaPixelSize;
        float4 Cop = SMAASampleLevelZero( colorTex, psIn.tex );
        float s = abs( offset.x ) > abs( offset.y )? abs( offset.x ) : abs( offset.y );

        // Unpack the velocity values:
        C.a *= C.a;
        Cop.a *= Cop.a;

        // Lerp the colors:
        float4 Caa = lerp( C, Cop, s );

        // Unpack velocity and return the resulting value:
        Caa.a = sqrt( Caa.a );
        return Caa;
        
        #else
        // Fetch the opposite color and lerp by hand:
        float4 C = SMAASampleLevelZero( colorTex, psIn.tex );
        psIn.tex += sign( offset ) * cb_smaaPixelSize;
        float4 Cop = SMAASampleLevelZero( colorTex, psIn.tex );
        float s = abs( offset.x ) > abs( offset.y ) ? abs( offset.x ) : abs( offset.y );
        return lerp( C, Cop, s );
        #endif
    }
}
