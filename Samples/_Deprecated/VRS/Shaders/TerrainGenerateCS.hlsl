//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"




// certainly lousy as a general purpose hash, but makes nice terrains
float RockyTerrainHash(float2 value)
{
	float3 hash = frac((1.0 / 3.14159265359) * float3(value.xy, value.x + value.y));
	hash += 3.14159265359 * dot(hash, hash.yzx);
	return frac(dot(hash, hash.zzz));
}


// 2D value noise and its analytical derivatives, from http://iquilezles.org/www/articles/morenoise/morenoise.htm
float3 noised(float2 pos)
{
	float2 p = floor(pos);
	float a = RockyTerrainHash(p + float2(0.5, 0.5));
	float b = RockyTerrainHash(p + float2(1.5, 0.5));
	float c = RockyTerrainHash(p + float2(0.5, 1.5));
	float d = RockyTerrainHash(p + float2(1.5, 1.5));

	float2 f = frac(pos);
	float2 u = f*f*f*(f*(f*6.0 - 15.0) + 10.0);

	float k0 = b - a;
	float k1 = c - a;
	float k2 = a - b - c + d;

	return float3(	a + (k0*u.x) + (k1*u.y) + (k2*u.x*u.y),			// value
					6.0*f*(1.0 - f)*(float2(k0, k1) + k2*u.yx));	// derivatives
}


float GetHeight(float2 pos)
{
    float lacunarity = 2.0;
    float gain = 0.5;
    int octaves = 16;

    const float2x2 m2 = float2x2(0.8, -0.6, 0.6, 0.8);
	float2 freq = float2(0.0, 0.0);
	float sum = 0.0;
	float amp = 1.0;

	for (int i = 0; i < octaves; i++)
	{
		float3 noise = noised(pos);
        freq += noise.yz;
        sum += amp * noise.x / (1.0 + dot(freq, freq));
        amp *= gain;
		pos = mul(pos, m2) * lacunarity;
	}
	return sum;
}


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]
void TerrainGenerate(uint3 threadId : SV_DispatchThreadID)
{
    float2 offset = float2(10.0, 0.0);
    float scale = 0.0025;
    
    float height = GetHeight(offset + float2(threadId.xy) * scale * sourceToHeightMapSizeMultiplier);
    height *= 0.5;
    height *= height;
    height *= worldScaleY;

    SourceHeightMapUAV0[threadId.xy] = max(0.01, height + 0.01);   // zero height is used as an optimisation to denotes out of range
}

