//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

// SamplerStates
#ifdef SUPPORT_NORMAL_MAP
SamplerState g_samNormal : register(s0);
#else
SamplerState g_samDiffuse : register(s0);
#endif

// Textures
Texture2D<float4> g_texDiffuse : register(t1);
#ifdef SUPPORT_NORMAL_MAP
Texture2D<float3> g_texNormal : register(t2);
#endif

// For shading calculations
ConstantBuffer<CBLightStruct> cbLight : register(b1);

#ifdef SUPPORT_NORMAL_MAP
//-------------------------------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform
//-------------------------------------------------------------------------------------------------------------
float3 CalcPerPixelNormal(in float2 vTexcoord, in float3 vVertNormal, in float3 vVertTangent)
{
	// Compute tangent frame
    vVertNormal = normalize(vVertNormal);
    vVertTangent = normalize(vVertTangent);
    float3 vVertBinormal = normalize(cross(vVertTangent, vVertNormal));
    float3x3 mTangentSpaceToWorldSpace = float3x3(vVertTangent, vVertBinormal, vVertNormal);
	
	// Compute per-pixel normal
    float3 vBumpNormal = g_texNormal.Sample(g_samNormal, vTexcoord).xyz;
    vBumpNormal = 2.0f * vBumpNormal - 1.0f;
	
    return mul(vBumpNormal, mTangentSpaceToWorldSpace);
}
#endif

[RootSignature(ROOT_SIG)]
Pixel main(InterpolantsMesh In)
{
    Pixel Out;
    
    float3 vNormal;
#ifdef SUPPORT_NORMAL_MAP
    vNormal = CalcPerPixelNormal(In.texcoord, In.normal, In.tangent);
#else
    vNormal = normalize(In.normal);
#endif

    // diffuse
    float fDiffuseIntensity = saturate(-dot(cbLight.g_LightData[0].m_vLightWorldDir.xyz, vNormal));

        // specular
    float3 vReflect = reflect(cbLight.g_LightData[0].m_vLightWorldDir.xyz, vNormal);
    float3 vEyeToSurface = normalize(In.posworld - cbLight.g_vEye.xyz);
    const float fEpsilon = 1e-10; // pow( 0.0f, 0.0f ) can explode
    float fSpecularIntensity = pow(saturate(-dot(vReflect, vEyeToSurface)) + fEpsilon, cbLight.g_LightData[0].m_fSpecularPower);

        // total
    Out.color = (fDiffuseIntensity + fSpecularIntensity) * cbLight.g_LightData[0].m_vLightColor;
    
#ifdef SUPPORT_NORMAL_MAP
    Out.color *= g_texDiffuse.Sample(g_samNormal, In.texcoord);
#else
    Out.color *= g_texDiffuse.Sample( g_samDiffuse, In.texcoord );
#endif
    
    return Out;
}
