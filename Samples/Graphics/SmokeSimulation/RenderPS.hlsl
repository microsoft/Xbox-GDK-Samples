//--------------------------------------------------------------------------------------
// RenderPS.hlsl
//
// Simple pixel shader for rendering volumetric data
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RenderCommon.hlsli"

//--------------------------------------------------------------------------------------
// Helper function which determines whether a ray intersects with a box

inline bool intersectBox(Ray r, float3 boxmin, float3 boxmax, out float tnear, out float tfar)
{
    // compute intersection of ray with all six bbox planes
    float3 invR = float3(1.0f, 1.0f, 1.0f) / r.d;
    float3 tbot = invR * (boxmin - r.o);
    float3 ttop = invR * (boxmax - r.o);

    // re-order intersections to find smallest and largest on each axis
    float3 tmin = min(ttop, tbot);
    float3 tmax = max(ttop, tbot);

    // find the largest tmin and the smallest tmax
    float largest_tmin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallest_tmax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));

	tnear = largest_tmin;
	tfar = smallest_tmax;

	return smallest_tmax > largest_tmin;
}

//--------------------------------------------------------------------------------------
// Pixel shader
// Renders the 3D volume using a simple ray-marching algorithm

[RootSignature(RenderRS)]
float4 main( PS_INPUT input) : SV_Target
{
    // Visualize cube boundary
    // Please note, this is not the ideal way of rendering cube boundary, it creates lots of aliasing.
    // However it is an approach with the least amount of code required under the current setting of this sample code.
    // For appropriate line rendering, use line primitives
    if ( (input.WorldPos.x <= -0.995 && input.WorldPos.y <= -0.995) ||
	     (input.WorldPos.x <= -0.995 && input.WorldPos.y >=  0.995) ||
		 (input.WorldPos.x >=  0.995 && input.WorldPos.y <= -0.995) ||
		 (input.WorldPos.x >=  0.995 && input.WorldPos.y >=  0.995) || 

		 (input.WorldPos.x <= -0.995 && input.WorldPos.z <= -0.995) ||
	     (input.WorldPos.x <= -0.995 && input.WorldPos.z >=  0.995) ||
		 (input.WorldPos.x >=  0.995 && input.WorldPos.z <= -0.995) ||
		 (input.WorldPos.x >=  0.995 && input.WorldPos.z >=  0.995) || 

		 (input.WorldPos.z <= -0.995 && input.WorldPos.y <= -0.995) ||
	     (input.WorldPos.z <= -0.995 && input.WorldPos.y >=  0.995) ||
		 (input.WorldPos.z >=  0.995 && input.WorldPos.y <= -0.995) ||
		 (input.WorldPos.z >=  0.995 && input.WorldPos.y >=  0.995) 		 
	   ) 
		 return float4(0.2, 0.2, 0.2, 1);
		    
    // Cast a ray from eye position to the point on the cube this pixel shader is executing on
    Ray r;
    r.o = EyePosition.xyz;
    r.d = normalize(input.WorldPos - r.o);
    
    // Examine whether the ray intersects with our 3D volume, exit early if it doesn't
    float tnear, tfar;
    bool hit = intersectBox( r, float3(-1, -1, -1), float3(1, 1, 1), tnear, tfar );
    if ( !hit )
        return float4(0, 0, 0, 0);
    
    const float tstep = 0.01f;
    float t = tnear;    
    float3 coord = (r.o + r.d * tnear + float3(1.0f, 1.0f, 1.0f)) / 2.0f;
    float3 step = r.d * tstep / 2.0f;

    float sum = 0.0f;
    float a = 0.0f;

    // Integrates along the ray while sampling the 3D volume
    for ( int i = 0; i < 500; ++i )
    {        
        // Simple acceleration technique: sample the 1/8 mip lod
        float col = g_tex3Dmip.SampleLevel( g_sam, coord, 0 ).x;
            
        // and skip chunks in space if it is empty, this technique gives us ~10% boost
        if ( col < 0.01 )
        {
            t += tstep * 8;
            if ( t > tfar )
                break;

            coord += step * 8;
            continue;
        }

        // if it is not empty, sample the 3D volume and accumulate color
        col = g_tex3D.SampleLevel( g_sam, coord, 0 ).x;
        sum += col * ( col / 10 ) * ( 1 - a );
        a += ( col / 10 ) * ( 1 - a );

        // Exit early if color already saturates
        if ( sum >= 1.0f || a >= 1.0f)
            break;

        // Exit early if going out of the 3D volume
        t += tstep;
        if ( t > tfar )
            break;

        coord += step;            
    }
        
    return float4( sum, sum, sum, a );            
}
