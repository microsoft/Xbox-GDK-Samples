///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         (see XeGTAO.h)                                                                            (='.'=)
// Details:         https://github.com/GameTechDev/XeGTAO                                                     (")_(")
//
// Version history: see XeGTAO.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#endif

#define XE_GTAO_HILBERT_LUT_AVAILABLE 1

#ifndef __INTELLISENSE__    // avoids some pesky intellisense errors
#include "XeGTAO.h"
#endif

cbuffer GTAOConstantBuffer                      : register(b0)
{
    GTAOConstants               g_GTAOConsts;
}

#include "XeGTAO.hlsli"
#include "GTAORS.hlsli"

// input output textures for the second pass (XeGTAO_MainPass)
Texture2D<lpfloat>          g_srcWorkingDepth       : register(t0);   // viewspace depth with MIPs, output by XeGTAO_PrefilterDepths16x16 and consumed by XeGTAO_MainPass
Texture2D<float3>           g_srcNormalmap          : register(t1);   // source normal map (if used)
#ifdef XE_GTAO_HILBERT_LUT_AVAILABLE
Texture2D<uint>             g_srcHilbertLUT         : register(t2);   // hilbert lookup table  (if any)
#endif
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
RWTexture2D<uint>           g_outWorkingAOTerm      : register(u0);   // output AO term (includes bent normals if enabled - packed as R11G11B10 scaled by AO)
#else
RWTexture2D<float>          g_outWorkingAOTerm      : register(u0);   // output AO term (includes bent normals if enabled - packed as R11G11B10 scaled by AO)
#endif
RWTexture2D<uint>           g_outWorkingEdges       : register(u1);   // output depth-based edges used by the denoiser
// g_samplerPointClamp is a sampler with D3D12_FILTER_MIN_MAG_MIP_POINT filter and D3D12_TEXTURE_ADDRESS_MODE_CLAMP addressing mode
SamplerState                g_samplerPointClamp : register(s0);


#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
void XeGTAO_OutputWorkingTerm( const uint2 pixCoord, lpfloat visibility, lpfloat3 bentNormal)
#else
void XeGTAO_OutputWorkingTerm(const uint2 pixCoord, lpfloat visibility)
#endif
{
    visibility = saturate(visibility / lpfloat(XE_GTAO_OCCLUSION_TERM_SCALE));
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
    g_outWorkingAOTerm[pixCoord] = XeGTAO_EncodeVisibilityBentNormal( visibility, bentNormal );
#else
    g_outWorkingAOTerm[pixCoord] = visibility;
#endif
}

// "Efficiently building a matrix to rotate one vector to another"
// http://cs.brown.edu/research/pubs/pdfs/1999/Moller-1999-EBA.pdf / https://dl.acm.org/doi/10.1080/10867651.1999.10487509
// (using https://github.com/assimp/assimp/blob/master/include/assimp/matrix3x3.inl#L275 as a code reference as it seems to be best)
lpfloat3x3 XeGTAO_RotFromToMatrix(lpfloat3 from, lpfloat3 to)
{
    const lpfloat e = dot(from, to);
    const lpfloat f = abs(e); //(e < 0)? -e:e;

    // WARNING: This has not been tested/worked through, especially not for 16bit floats; seems to work in our special use case (from is always {0, 0, -1}) but wouldn't use it in general
    if (f > lpfloat(1.0 - 0.0003))
        return lpfloat3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

    const lpfloat3 v = cross(from, to);
    // ... use this hand optimized version (9 mults less) 
    const lpfloat h = (1.0) / (1.0 + e); // optimization by Gottfried Chen 
    const lpfloat hvx = h * v.x;
    const lpfloat hvz = h * v.z;
    const lpfloat hvxy = hvx * v.y;
    const lpfloat hvxz = hvx * v.z;
    const lpfloat hvyz = hvz * v.y;

    lpfloat3x3 mtx;
    mtx[0][0] = e + hvx * v.x;
    mtx[0][1] = hvxy - v.z;
    mtx[0][2] = hvxz + v.y;

    mtx[1][0] = hvxy + v.z;
    mtx[1][1] = e + h * v.y * v.y;
    mtx[1][2] = hvyz - v.x;

    mtx[2][0] = hvxz - v.y;
    mtx[2][1] = hvyz + v.x;
    mtx[2][2] = e + hvz * v.z;

    return mtx;
}

// Engine-specific screen & temporal noise loader
lpfloat2 SpatioTemporalNoise(uint2 pixCoord, uint temporalIndex)    // without TAA, temporalIndex is always 0
{
    float2 noise;
#if 1   // Hilbert curve driving R2 (see https://www.shadertoy.com/view/3tB3z3)
#ifdef XE_GTAO_HILBERT_LUT_AVAILABLE // load from lookup texture...
    uint index = g_srcHilbertLUT.Load(uint3(pixCoord % 64, 0)).x;
#else // ...or generate in-place?
    uint index = HilbertIndex(pixCoord.x, pixCoord.y);
#endif
    index += temporalIndex;
    // R2 sequence - see http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    return lpfloat2(frac(0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114)));
#else   // Pseudo-random (fastest but looks bad - not a good choice)
    uint baseHash = Hash32(pixCoord.x + (pixCoord.y << 15));
    baseHash = Hash32Combine(baseHash, temporalIndex);
    return lpfloat2(Hash32ToFloat(baseHash), Hash32ToFloat(Hash32(baseHash)));
#endif
}

// Engine-specific normal map loader
lpfloat3 LoadNormal(int2 pos)
{
#if 0
    // special decoding for external normals stored in 11_11_10 unorm - modify appropriately to support your own encoding 
    uint packedInput = g_srcNormalmap.Load(int3(pos, 0)).x;
    float3 unpackedOutput = XeGTAO_R11G11B10_UNORM_to_FLOAT3(packedInput);
    float3 normal = normalize(unpackedOutput * 2.0.xxx - 1.0.xxx);
#else 
    // example of a different encoding
    float3 encodedNormal = g_srcNormalmap.Load(int3(pos, 0)).xyz;
    float3 normal = normalize(encodedNormal * 2.0.xxx - 1.0.xxx);
#endif

#if 0 // compute worldspace to viewspace here if your engine stores normals in worldspace; if generating normals from depth here, they're already in viewspace
    normal = mul((float3x3)g_globals.View, normal);
#endif

    return (lpfloat3) normal;
}


#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
void XeGTAO_MainPass( const uint2 pixCoord, lpfloat sliceCount, lpfloat stepsPerSlice)
#else
void XeGTAO_MainPass(const uint2 pixCoord, lpfloat sliceCount, lpfloat stepsPerSlice)
#endif
{
    float2 normalizedScreenPos = (pixCoord + 0.5.xx) * g_GTAOConsts.ViewportPixelSize;

    lpfloat4 valuesUL = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord * g_GTAOConsts.ViewportPixelSize));

    // viewspace Z at the center
    lpfloat viewspaceZ = valuesUL.y; // g_srcWorkingDepth.SampleLevel( g_samplerPointClamp, normalizedScreenPos, 0 ).x;

    // Move center pixel slightly towards camera to avoid imprecision artifacts due to depth buffer imprecision; offset depends on depth texture format used
#ifdef XE_GTAO_FP32_DEPTHS
    viewspaceZ *= 0.99999; // this is good for FP32 depth buffer
#else
    viewspaceZ *= 0.99920; // this is good for FP16 depth buffer
#endif

#if XE_GTAO_USE_DEFAULT_CONSTANTS != 0
    const lpfloat sampleDistributionPower = (lpfloat) XE_GTAO_DEFAULT_SAMPLE_DISTRIBUTION_POWER;
    const lpfloat thinOccluderCompensation = (lpfloat) XE_GTAO_DEFAULT_THIN_OCCLUDER_COMPENSATION;
#else
    const lpfloat sampleDistributionPower   = (lpfloat)g_GTAOConsts.SampleDistributionPower;
    const lpfloat thinOccluderCompensation  = (lpfloat)g_GTAOConsts.ThinOccluderCompensation;
#endif

    // fadeout precompute optimisation
    const float falloffMul = g_GTAOConsts.FalloffMul;
    const float falloffAdd = g_GTAOConsts.FalloffAdd;

    // see "Algorithm 1" in https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
    const lpfloat2 localNoise = SpatioTemporalNoise(pixCoord, g_GTAOConsts.NoiseIndex);
    const lpfloat noiseSlice = localNoise.x;
    const lpfloat noiseSample = localNoise.y;

    // quality settings / tweaks / hacks
    const lpfloat pixelTooCloseThreshold = 1.3; // if the offset is under approx pixel size (pixelTooCloseThreshold), push it out to the minimum distance

    // approx viewspace pixel size at pixCoord; approximation of NDCToViewspace( normalizedScreenPos.xy + g_GTAOConsts.ViewportPixelSize.xy, pixCenterPos.z ).xy - pixCenterPos.xy;
    const float pixelDirRBViewspaceSizeAtCenterZ = viewspaceZ * g_GTAOConsts.NDCToViewMul_x_PixelSize.x;

    lpfloat screenspaceRadius = lpfloat(g_GTAOConsts.EffectRadiusScaled / pixelDirRBViewspaceSizeAtCenterZ);

    // fade out for small screen radii 
    lpfloat visibility = saturate((10 - screenspaceRadius) / 100) * 0.5;

#if 0   // sensible early-out for even more performance; disabled because not yet tested
    [branch]
    if( screenspaceRadius < pixelTooCloseThreshold )
    {
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
        XeGTAO_OutputWorkingTerm( pixCoord, 1, LoadNormal(pixCoord));
#else
        XeGTAO_OutputWorkingTerm( pixCoord, 1);
#endif
        return;
    }
#endif

    // this is the min distance to start sampling from to avoid sampling from the center pixel (no useful data obtained from sampling center pixel)
    const lpfloat minS = pixelTooCloseThreshold / screenspaceRadius;

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
    lpfloat3 bentNormal = 0;
#endif

    // viewspace Zs left top right bottom
    lpfloat4 valuesBR = g_srcWorkingDepth.GatherRed(g_samplerPointClamp, float2(pixCoord * g_GTAOConsts.ViewportPixelSize), int2(1, 1));
    const lpfloat pixCenter = valuesUL.y;
    const lpfloat pixLZ = valuesUL.x;
    const lpfloat pixTZ = valuesUL.z;
    const lpfloat pixRZ = valuesBR.z;
    const lpfloat pixBZ = valuesBR.x;

    lpfloat4 edgesLRTB = XeGTAO_CalculateEdges(pixCenter, pixLZ, pixRZ, pixTZ, pixBZ);
    g_outWorkingEdges[pixCoord] = XeGTAO_PackEdges(edgesLRTB);

#ifdef XE_GTAO_SHOW_EDGES
    g_outputDbgImage[pixCoord] = 1.0 - float4( edgesLRTB.x, edgesLRTB.y * 0.5 + edgesLRTB.w * 0.5, edgesLRTB.z, 1.0 );
#endif

    // Generating screen space normals in-place is faster than generating normals in a separate pass but requires
	// use of 32bit depth buffer (16bit works but visibly degrades quality) which in turn slows everything down. So to
	// reduce complexity and allow for screen space normal reuse by other effects, we've pulled it out into a separate
	// pass.
	// However, we leave this code in, in case anyone has a use-case where it fits better.
#ifdef XE_GTAO_GENERATE_NORMALS_INPLACE
    float3 CENTER   = XeGTAO_ComputeViewspacePosition( normalizedScreenPos, pixCenter, g_GTAOConsts );
    float3 LEFT     = XeGTAO_ComputeViewspacePosition( normalizedScreenPos + float2(-1,  0) * g_GTAOConsts.ViewportPixelSize, pixLZ, g_GTAOConsts );
    float3 RIGHT    = XeGTAO_ComputeViewspacePosition( normalizedScreenPos + float2( 1,  0) * g_GTAOConstsViewportPixelSize, pixRZ, g_GTAOConsts );
    float3 TOP      = XeGTAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0, -1) * g_GTAOConsts.ViewportPixelSize, pixTZ, g_GTAOConsts );
    float3 BOTTOM   = XeGTAO_ComputeViewspacePosition( normalizedScreenPos + float2( 0,  1) * g_GTAOConsts.ViewportPixelSize, pixBZ, g_GTAOConsts );
    viewspaceNormal = (lpfloat3)XeGTAO_CalculateNormal( edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM );
#endif

    const float3 pixCenterPos = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ, g_GTAOConsts);
    const lpfloat3 viewVec = (lpfloat3) normalize(-pixCenterPos);
    lpfloat3 viewspaceNormal = LoadNormal(pixCoord);
    
    // prevents normals that are facing away from the view vector - xeGTAO struggles with extreme cases, but in Vanilla it seems rare so it's disabled by default
    // viewspaceNormal = normalize( viewspaceNormal + max( 0, -dot( viewspaceNormal, viewVec ) ) * viewVec );

#ifdef XE_GTAO_SHOW_NORMALS
    g_outputDbgImage[pixCoord] = float4( DisplayNormalSRGB( viewspaceNormal.xyz ), 1 );
#endif

    [unroll]
    for (lpfloat slice = 0; slice < sliceCount; slice++)
    {
        lpfloat sliceK = (slice + noiseSlice) / sliceCount;
        // lines 5, 6 from the paper
        lpfloat phi = sliceK * XE_GTAO_PI;
        lpfloat2 cosSinPhi = lpfloat2(cos(phi), sin(phi));
        lpfloat2 omega = cosSinPhi * lpfloat2(1, -1); //lpfloat2 on omega causes issues with big radii

        // convert to screen units (pixels) for later use
        omega *= screenspaceRadius;

        // line 8 from the paper
        const lpfloat2 directionVec = cosSinPhi; // cos(phi), sin(phi), 0   
        const lpfloat2 mulDirView = directionVec * viewVec.xy;
        const lpfloat dotDirView = mulDirView.x + mulDirView.y;
        const lpfloat2 orthoDirVecXY = directionVec - dotDirView * viewVec.xy;
        
        // line 9 from the paper
        const lpfloat3 orthoDirectionVec = lpfloat3(orthoDirVecXY, -(dotDirView * viewVec.z));

        // line 10 from the paper
        //axisVec is orthogonal to directionVec and viewVec, used to define projectedNormal
        const lpfloat3 crossVec = cross(orthoDirectionVec, lpfloat3(viewVec.xy, viewVec.z));
        lpfloat2 mulCrossXY = crossVec.xy * crossVec.xy;
        lpfloat dotCrossVec = mulCrossXY.x + mulCrossXY.y + crossVec.z * crossVec.z;
        const lpfloat3 axisVec = rsqrt(dotCrossVec) * crossVec;

        // alternative line 9 from the paper
        // float3 orthoDirectionVec = cross( viewVec, axisVec );

        // line 11 from the paper
        lpfloat2 mulViewAxis = viewspaceNormal.xy * axisVec.xy;
        lpfloat dotViewAxis = mulViewAxis.x + mulViewAxis.y + viewspaceNormal.z * axisVec.z;
        lpfloat3 projectedNormalVec = viewspaceNormal - axisVec * dotViewAxis;

        // line 13 from the paper
        lpfloat signNorm = (lpfloat) sign(dot(orthoDirectionVec, projectedNormalVec));

        // line 14 from the paper
        lpfloat projectedNormalVecLength = length(projectedNormalVec);
        lpfloat cosNorm = saturate(dot(projectedNormalVec, viewVec) / projectedNormalVecLength);

        // line 15 from the paper
        lpfloat n = signNorm * XeGTAO_FastACos(cosNorm);

        // this is a lower weight target; not using -1 as in the original paper because it is under horizon, so a 'weight' has different meaning based on the normal
        // From https://en.wikipedia.org/wiki/List_of_trigonometric_identities 
        // cos(pi/2 - theta) = sin(theta)
        // cos(-theta) = cos(theta)
        // sin(-theta) = -sin(theta)
        const lpfloat sinN = sin(n);
        const lpfloat2 lowHorizonCos = lpfloat2(-sinN, sinN); // cos(n + XE_GTAO_PI_HALF), cos(n - XE_GTAO_PI_HALF)

        // lines 17, 18 from the paper, manually unrolled the 'side' loop
        lpfloat2 horizonCos = lowHorizonCos;

        [unroll]
        for (lpfloat step = 0; step < stepsPerSlice; step++)
        {
            // R1 sequence (http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/)
            const lpfloat stepBaseNoise = lpfloat(slice + step * stepsPerSlice) * 0.6180339887498948482; // <- this should unroll
            lpfloat stepNoise = frac(noiseSample + stepBaseNoise);

            // approx line 20 from the paper, with added noise
            lpfloat s = (step + stepNoise) / (stepsPerSlice); // + (lpfloat2)1e-6f);

            // additional distribution modifier
            s = pow(s, sampleDistributionPower);

            // avoid sampling center pixel
            s += minS;

            // approx lines 21-22 from the paper, unrolled
            lpfloat2 sampleOffset = s * omega;

            lpfloat sampleOffsetLength = length(sampleOffset);

            // note: when sampling, using point_point_point or point_point_linear sampler works, but linear_linear_linear will cause unwanted interpolation between neighbouring depth values on the same MIP level!
            const float mipLevel = clamp(log2(sampleOffsetLength) - g_GTAOConsts.DepthMIPSamplingOffset, 0, XE_GTAO_DEPTH_MIP_LEVELS);

            // Snap to pixel center (more correct direction math, avoids artifacts due to sampling pos not matching depth texel center - messes up slope - but adds other 
            // artifacts due to them being pushed off the slice). Also use full precision for high res cases.
            sampleOffset = round(sampleOffset) * (lpfloat2) g_GTAOConsts.ViewportPixelSize;

            float2 sampleScreenPos0 = normalizedScreenPos + sampleOffset;
            float SZ0 = g_srcWorkingDepth.SampleLevel(g_samplerPointClamp, sampleScreenPos0, mipLevel).x;
            float3 samplePos0 = XeGTAO_ComputeViewspacePosition(sampleScreenPos0, SZ0, g_GTAOConsts);

            float2 sampleScreenPos1 = normalizedScreenPos - sampleOffset;
            float SZ1 = g_srcWorkingDepth.SampleLevel(g_samplerPointClamp, sampleScreenPos1, mipLevel).x;
            float3 samplePos1 = XeGTAO_ComputeViewspacePosition(sampleScreenPos1, SZ1, g_GTAOConsts);

            float3 sampleDelta0 = (samplePos0 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
            float3 sampleDelta1 = (samplePos1 - float3(pixCenterPos)); // using lpfloat for sampleDelta causes precision issues
            float2 sampleDist = float2(length(sampleDelta0), length(sampleDelta1));

            // approx lines 23, 24 from the paper, unrolled
            lpfloat2 sampleHorizonVecX = lpfloat2(sampleDelta0.x / sampleDist.x, sampleDelta1.x / sampleDist.y);
            lpfloat2 sampleHorizonVecY = lpfloat2(sampleDelta0.y / sampleDist.x, sampleDelta1.y / sampleDist.y);
            lpfloat2 sampleHorizonVecZ = lpfloat2(sampleDelta0.z / sampleDist.x, sampleDelta1.z / sampleDist.y);

            // any sample out of radius should be discarded - also use fallof range for smooth transitions; this is a modified idea from "4.3 Implementation details, Bounding the sampling area"
#if XE_GTAO_USE_DEFAULT_CONSTANTS != 0 && XE_GTAO_DEFAULT_THIN_OBJECT_HEURISTIC == 0
            lpfloat2 weight = (lpfloat2)saturate( sampleDist * falloffMul + falloffAdd );
#else
            // this is our own thickness heuristic that relies on sooner discarding samples behind the center
            lpfloat falloffBase0 = length(lpfloat3(sampleDelta0.x, sampleDelta0.y, sampleDelta0.z * (1 + thinOccluderCompensation)));
            lpfloat falloffBase1 = length(lpfloat3(sampleDelta1.x, sampleDelta1.y, sampleDelta1.z * (1 + thinOccluderCompensation)));
            lpfloat2 falloffBase = lpfloat2(falloffBase0, falloffBase1);
            lpfloat2 weight = saturate(falloffBase * falloffMul + falloffAdd);
#endif

            // sample horizon cos
            lpfloat2 shc = sampleHorizonVecX * viewVec.x + sampleHorizonVecY * viewVec.y + sampleHorizonVecZ * viewVec.z;

            // discard unwanted samples
            shc = lerp(lowHorizonCos, shc, weight);

            // thickness heuristic - see "4.3 Implementation details, Height-field assumption considerations"
#if 0       // (disabled, not used) this should match the paper
            lpfloat2 newhorizonCos = max(horizonCos, shc);
            horizonCos = select((horizonCos > shc), lerp( newhorizonCos, shc, thinOccluderCompensation ), newhorizonCos);
#elif 0     // (disabled, not used) this is slightly different from the paper but cheaper and provides very similar results
            horizonCos = lerp( max( horizonCos, shc ), shc, thinOccluderCompensation );
#else       // this is a version where thicknessHeuristic is completely disabled
            horizonCos = max(horizonCos, shc);
#endif
        }

#if 1   // I can't figure out the slight overdarkening on high slopes, so I'm adding this fudge - in the training set, 0.05 is close (PSNR 21.34) to disabled (PSNR 21.45)
        projectedNormalVecLength = lerp(projectedNormalVecLength, 1, 0.05);
#endif

        // line ~27, unrolled
        lpfloat2 h = lpfloat2(XeGTAO_FastACos(horizonCos) * lpfloat2(1, -1));
#if 0   // we can skip clamping for a tiny little bit more performance
        h = n + clamp( h-n, (lpfloat)-XE_GTAO_PI_HALF, (lpfloat)XE_GTAO_PI_HALF );
#endif
        lpfloat2 iarc = (cosNorm + 2 * h * sin(n) - cos(2 * h - n)) / 4;
        lpfloat localVisibility = projectedNormalVecLength * (iarc.x + iarc.y);
        visibility += localVisibility;

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
        // see "Algorithm 2 Extension that computes bent normals b."
        lpfloat t0 = (6 * sin(h.y - n) - sin(3 * h.y - n) + 6 * sin(h.x - n) - sin(3 * h.x - n) + 16 * sin(n) - 3 * (sin(h.y + n) + sin(h.x + n))) / 12;
        lpfloat t1 = (-cos(3 * h.y - n) - cos(3 * h.x - n) + 8 * cos(n) - 3 * (cos(h.y + n) + cos(h.x + n))) / 12;
        lpfloat3 localBentNormal = lpfloat3(directionVec.x * (lpfloat) t0, directionVec.y * (lpfloat) t0, -lpfloat(t1));
        localBentNormal = (lpfloat3) mul(XeGTAO_RotFromToMatrix(lpfloat3(0, 0, -1), viewVec), localBentNormal) * projectedNormalVecLength;
        bentNormal += localBentNormal;
#endif
    }
    visibility /= (lpfloat) sliceCount;
    visibility = exp2(lpfloat(g_GTAOConsts.FinalValuePower * log2(visibility))); // equivalent to pow but allows for better code gen in fp16
    visibility = max((lpfloat) 0.03, visibility); // disallow total occlusion (which wouldn't make any sense anyhow since pixel is visible but also helps with packing bent normals)

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
    bentNormal = normalize(bentNormal) ;
    XeGTAO_OutputWorkingTerm(pixCoord, visibility, bentNormal);
#else
    XeGTAO_OutputWorkingTerm(pixCoord, visibility);
#endif
}
