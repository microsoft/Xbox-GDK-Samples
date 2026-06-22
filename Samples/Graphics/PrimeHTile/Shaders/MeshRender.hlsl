//--------------------------------------------------------------------------------------
// MeshRender.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define MeshRenderRS \
    "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "            DENY_HULL_SHADER_ROOT_ACCESS )," \
    "DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
    "DescriptorTable ( SRV(t1), visibility = SHADER_VISIBILITY_PIXEL ),"\
    "CBV(b0), "\
    "StaticSampler(s0,"\
    "           filter = FILTER_ANISOTROPIC,"\
    "           addressU = TEXTURE_ADDRESS_WRAP,"\
    "           addressV = TEXTURE_ADDRESS_WRAP,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           maxAnisotropy = 16,"\
    "           comparisonFunc = COMPARISON_ALWAYS,"\
    "           visibility = SHADER_VISIBILITY_PIXEL )"

cbuffer MainSceneCB : register( b0 )
{
    float4x4 worldMatrix;
    float4x4 worldViewProjectionMatrix;
	float4   eyePositionOS;		// object space
    float4   lightDirOS;		// object space
    float4   lightColor;
	float	 mainRenderScale;
	float	 occlusionScale;
}

Texture2D    normalMap : register( t0 );
Texture2D    heightMap : register( t1 );
sampler s0: register(s0);


struct DEPTH_ONLY_VS_INPUT
{
	float4 pos : SV_POSITION;
	float4 positionAndScale : I_TRANSFORM;
};

struct VS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangentAndBinnormalFlip : TANGENT;
    float2 uv : UV;
	float4 positionAndScale : I_TRANSFORM;
};

struct PARALLAX_MAPPING_VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 eyeSteps : UV0;
	float3 uvFog : UV1;
	float3x3 nbt : NBT;
};

struct BUMP_MAPPING_VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uvFog : UV1;
	float3x3 nbt : NBT;
};

static const float		g_UVScale = 4.0;

static const float		g_FogStart = 1.0;
static const float		g_FogStrength = 0.1;

static const float		g_HeightMapScale = 0.125;
static const int		g_MaxSampleCount = 12;
static const int		g_MinSampleCount = 6;



//--------------------------------------------------------------------------------------
// Parallax mapping implementation
//--------------------------------------------------------------------------------------
float2 ParallaxMappingAdjustUV( float2 uv, float3 eye, float lerpSampleCount )
{
	int sampleCount			= (int)lerp(g_MaxSampleCount, g_MinSampleCount, lerpSampleCount);
	float invSampleCount	= 1.0 / (float)sampleCount;
	float2 uvStep			= float2(-eye.x, eye.y) / (eye.z * sampleCount);

	float testHeight		= 1.0;	
	float prevHeightDelta	= 1.0;
	float2 dx				= ddx( uv );
	float2 dy				= ddy( uv );

	for ( int i = 0; i < sampleCount; ++i )
	{
		float height		 = heightMap.SampleGrad( s0, uv, dx, dy ).x;
		float heightDelta	 = height - testHeight;
				
		if ( heightDelta >= 0.0f )	
		{
			// linearly interpolate between samples above and below the 'surface'
			uv				 = lerp(uv, uv - uvStep, heightDelta / ( heightDelta - prevHeightDelta ) );
			break;
		}
		prevHeightDelta		 = heightDelta;
		testHeight			-= invSampleCount;
		uv					+= uvStep;
	}
	return uv;
}


//--------------------------------------------------------------------------------------
// Normal map decode and basic diffuse light
//--------------------------------------------------------------------------------------
float3 Shade(float2 uv, float3x3 nbt, float fog)
{
	// decode normal
	float3 normal;
	normal.xy = normalMap.Sample(s0, uv).xy;
	normal.z = 1.0 - sqrt(dot(normal.xy, normal.xy));
	normal = mul(normal, nbt);

	// lighting
	float lambert = saturate(dot(lightDirOS.xyz, normal));
	float3 color = lambert * lightColor.xyz;

	// apply fog
	return lerp(float3(0.2, 0.2, 0.7), color, fog);	
}


//--------------------------------------------------------------------------------------
// Parallax mapping pixel shader
//--------------------------------------------------------------------------------------
[RootSignature(MeshRenderRS)]
float4 ParallaxMappingPS( PARALLAX_MAPPING_VS_OUTPUT input ) : SV_Target
{
	float2 uv = ParallaxMappingAdjustUV(input.uvFog.xy, input.eyeSteps.xyz, input.eyeSteps.w);

	float3 color = Shade(uv, input.nbt, input.uvFog.z);

	return float4(color, 1.0);
}


//--------------------------------------------------------------------------------------
// Bump mapping pixel shader
//--------------------------------------------------------------------------------------
[RootSignature(MeshRenderRS)]
float4 BumpMappingPS(BUMP_MAPPING_VS_OUTPUT input) : SV_Target
{
	float3 color = Shade(input.uvFog.xy, input.nbt, input.uvFog.z);

	return float4(color, 1.0);
}


//--------------------------------------------------------------------------------------
// Limit UV's to minimum of one repeat
//--------------------------------------------------------------------------------------
float2 CalculateUV(float2 uv, float scale)
{
	return uv * max(scale, 1.0 / g_UVScale);
}


//--------------------------------------------------------------------------------------
// Simple exponential fog calculation
//--------------------------------------------------------------------------------------
float CalculateFog(float3 toEyeOS)
{
	float3 toEyeWS = mul(float4(toEyeOS, 1.0), worldMatrix).xyz;
	return saturate(exp((g_FogStart - length(toEyeWS)) * g_FogStrength));
}


//--------------------------------------------------------------------------------------
// Calculate 3x3 rotation matrix for tangent space calculations
//--------------------------------------------------------------------------------------
float3x3 CalculateNBT(VS_INPUT input)
{
	return float3x3(cross(input.tangentAndBinnormalFlip.xyz, input.normal) * input.tangentAndBinnormalFlip.w, input.tangentAndBinnormalFlip.xyz, input.normal);
}


//--------------------------------------------------------------------------------------
// Vertex shader needed for parallax mapping
//--------------------------------------------------------------------------------------
[RootSignature(MeshRenderRS)]
PARALLAX_MAPPING_VS_OUTPUT ParallaxMappingVS(VS_INPUT input)
{
	float scale = input.positionAndScale.w * mainRenderScale;
	float3 position = input.pos.xyz * scale + input.positionAndScale.xyz;
	float3 toEyeOS = eyePositionOS.xyz - position;

	PARALLAX_MAPPING_VS_OUTPUT output;
	output.pos = mul(float4(position, 1.0f), worldViewProjectionMatrix);
	output.uvFog.xy = CalculateUV(input.uv, scale);
	output.uvFog.z = CalculateFog(toEyeOS);
	output.nbt = CalculateNBT(input);

	float3x3 invNbt = transpose(output.nbt);
	output.eyeSteps.xyz = mul(toEyeOS, invNbt);
	output.eyeSteps.xy *= g_HeightMapScale;
	output.eyeSteps.w = saturate(dot(input.normal.xyz, normalize(toEyeOS)));

	return output;
}


//--------------------------------------------------------------------------------------
// Vertex shader needed for bump mapping
//--------------------------------------------------------------------------------------
[RootSignature(MeshRenderRS)]
BUMP_MAPPING_VS_OUTPUT BumpMappingVS(VS_INPUT input)
{
	float scale = input.positionAndScale.w * mainRenderScale;
	float3 position = input.pos.xyz * scale + input.positionAndScale.xyz;
	float3 toEyeOS = eyePositionOS.xyz - position;

	BUMP_MAPPING_VS_OUTPUT output;
	output.pos = mul(float4(position, 1.0f), worldViewProjectionMatrix);
	output.uvFog.xy = CalculateUV(input.uv, scale);
	output.uvFog.z = CalculateFog(toEyeOS);
	output.nbt = CalculateNBT(input);

	return output;
}


//--------------------------------------------------------------------------------------
// Vertex shader for outputting just depth
//--------------------------------------------------------------------------------------
[RootSignature(MeshRenderRS)]
float4 MinimumVS(DEPTH_ONLY_VS_INPUT input) : SV_POSITION
{
	float scale = input.positionAndScale.w * occlusionScale;
	float3 position = input.pos.xyz * scale + input.positionAndScale.xyz;

	return mul(float4(position, 1.0f), worldViewProjectionMatrix);
}

