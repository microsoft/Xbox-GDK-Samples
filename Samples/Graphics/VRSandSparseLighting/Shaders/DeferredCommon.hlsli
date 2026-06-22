//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Global.hlsli"

// can speed up encoding of pixel shaders. Obviously can't be assumed for decoding, not in a real game at least!
#ifndef NEGATIVE_Y_NORMAL_POSSIBLE
# define NEGATIVE_Y_NORMAL_POSSIBLE 1
#endif

// these two implementations are interesting, in the general case the 3 channel version wins on performance and accuracy, unsurprisingly as it uses more bits
// however in the case of a terrain which cannot output world space normals with a negative Y component, regular octant encoding is slightly faster 
#define NORMAL_ENCODING_OCTANT_2CHANNEL     0
#define NORMAL_ENCODING_OCTANT_3CHANNEL     1
#define NORMAL_ENCODING                     NORMAL_ENCODING_OCTANT_3CHANNEL         
//#define NORMAL_ENCODING                   NORMAL_ENCODING_OCTANT_2CHANNEL         


// Chose xz plane projection as for world space normals only normal.y > 0 can be said to normally take the fast path (due to sky not rendering to g-buffers)
// Maps the normal sphere to a diamond looking down the Y axis
float2 GetXZPlaneProjection(float3 normal)
{
#if NEGATIVE_Y_NORMAL_POSSIBLE == 1
    return normal.xz / (abs(normal.x) + abs(normal.y) + abs(normal.z));
#else
    return normal.xz / (abs(normal.x) + normal.y + abs(normal.z));
#endif
}


// Based on Octant encoding: On Floating Point Normal Vectors
// Quirin Meyer, Jochen Süßmuth, Gerd Sußner, Marc Stamminger, Günther Greiner
float2 NormalToOctEncode(float3 normal)
{
    float2 xzPlaneProjection = GetXZPlaneProjection(normal);
    float2 encode = xzPlaneProjection;

#if NEGATIVE_Y_NORMAL_POSSIBLE == 1
    [branch]
    if (normal.y < 0.0)    // this is typically the least common case, especially for terrains ;)
    {
        // reflect inner diamond to outer 4 triangles to use all of unit square
        encode  = 1.0 - abs(xzPlaneProjection.yx);
        encode *= select(xzPlaneProjection.xy >= 0.0, 1.0, -1.0); // don't use sign(xzPlanePorjection.xy) which can return 0, and is therefore more computationally expensive plus we don't want to multiply by zero!
    }
#endif
    return encode * 0.5 + 0.5;
}


float3 OctDecodeToNormal(float2 encode)
{
    encode = 2.0 * encode - 1.0;

    float3 normal = float3(encode.x, 1.0 - (abs(encode.x) + abs(encode.y)), encode.y);
    float maxNegY = saturate(-normal.y);

    normal.xz += select(normal.xz >= 0.0, -maxNegY, maxNegY);

    return normalize(normal);
}


// Based on John White's idea for leveraging 10102 of 1010102 format: http://johnwhite3d.blogspot.com/2017/10/signed-octahedron-normal-encoding.html
// The use of the 2bit channel for a Y sign removes the need for 'folding' the diamond from the +Y hemisphere to use the full unit square
float3 NormalToOct10102Encode(float3 normal)
{
    float2 xzPlaneProjection = GetXZPlaneProjection(normal);
#if NEGATIVE_Y_NORMAL_POSSIBLE == 1
    float z = saturate(normal.y * FLT_MAX);
#else
    float z = 1.0;
#endif
    return float3(xzPlaneProjection * 0.5 + 0.5, z);
}


float3 Oct10102DecodeToNormal(float3 encode)
{
    float3 normal = encode.xzy * 2.0 - 1.0;
    normal.y *= 1.0 - (abs(normal.x) + abs(normal.z));

    return normalize(normal);
}


float3 DecodeNormal(float4 gbuffer)
{
#if NORMAL_ENCODING == NORMAL_ENCODING_OCTANT_2CHANNEL
    return OctDecodeToNormal(gbuffer.yz);
#elif NORMAL_ENCODING == NORMAL_ENCODING_OCTANT_3CHANNEL
    return Oct10102DecodeToNormal(gbuffer.yzw);
#else
# error
#endif
}


float4 EncodeRoughnessNormal(float roughness, float3 normal)
{
#if NORMAL_ENCODING == NORMAL_ENCODING_OCTANT_2CHANNEL
    return float4(roughness, NormalToOctEncode(normal), 0.0);
#elif NORMAL_ENCODING == NORMAL_ENCODING_OCTANT_3CHANNEL
    return float4(roughness, NormalToOct10102Encode(normal));
#else
# error
#endif
}


float3 ComputeViewDirection(float2 uv)
{
    float3 dir = frustumOrigin.xyz;
    dir += uv.x * frustumHDelta.xyz;
    dir += uv.y * frustumVDelta.xyz;
    return normalize(dir);
}


// A lot less VALU than a 4x4 matrix multiply
float3 ComputeWorldPositionAndViewDirection(float2 uv, float dist01, out float3 dir)
{
    // frustum points are all on the far plane, so we only need distance stored01, not 0..farZ, meaning we can use unorm formats
    dir  = frustumOrigin.xyz;                   // scalar
    dir += uv.x * frustumHDelta.xyz;            // 3 madds
    dir += uv.y * frustumVDelta.xyz;            // 3 madds
    float3 pos = dist01 * dir + cameraPos.xyz;  // 3 madds

    dir = normalize(dir);

    return pos;
}
