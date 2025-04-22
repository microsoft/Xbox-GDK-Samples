//--------------------------------------------------------------------------------------
// LightBinningCompute.hlsl
//
// Implements the screen space Tiling algorithm.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "../shared.h"

// Structured buffer for light data
ConstantBuffer<LightPositions> lightcb : register(b0);

// G-Buffer SRVs
Texture2D<float4> g_texAlbedo : register(t1);
Texture2D<float3> g_texNormal : register(t2);
Texture2D<float3> g_texPos    : register(t3);
Texture2D<float>  g_texDepth  : register(t4);

struct lightOffsetConstant
{
    float r;
    float g;
    float b;
    float intensity;
};
ConstantBuffer<lightOffsetConstant> lightTints : register(b2);

// Constant buffer for the scene constants
ConstantBuffer<SceneConstants> sceneConstants : register(b1);

// Output UAV
RWTexture2D<float4> uav_out : register(u0);

// Each threadgroup here will run a thread per light, and see if that light belongs to this tile.
// If there are more than GROUP_WIDTHxGROUP_WIDTH lights, we just wrap.

// Shared memory
groupshared uint tileLightsCnt;
groupshared uint tileDepthMin;
groupshared uint tileDepthMax;
groupshared uint tileLightsIndices[MAX_LIGHTS_PER_TILE];

float3 ScreenToProjSpace(uint2 screenSpacePt, uint2 screenRes)
{
    float x = ((float)screenSpacePt.x / screenRes.x) * 2.0f - 1.0f;
    float y = ((float)screenSpacePt.y / screenRes.y) * 2.0f - 1.0f;
    return float3(x, y, 1.0f);
}

float4 ProjToViewSpace(float3 projSpacePt)
{
    float4 temp = mul(float4(projSpacePt, 1.0f), sceneConstants.invProj);
    temp = temp / temp.w;
    return temp;
}

// Plane consists on 2 points plus origin, so we calculate normal and then fustrum test
bool TestSphereWithFustrumPlane(float3 p1, float3 p2, float3 center, float radius)
{
    // Get the normal with cross product
    // DirectX uses left hand rule - z points towards screen.
    float3 planeNormal = cross(p1, p2);
    planeNormal = normalize(planeNormal);

    // Test the normal against our point
    return dot(center, planeNormal) < radius;
}

[RootSignature(ComputeRS)]
[numthreads(GROUP_WIDTH, GROUP_WIDTH, 1)]
void main( uint3 DTid : SV_DispatchThreadID,
           uint3 gTid : SV_GroupThreadID,
           uint3 gId  : SV_GroupID )
{
    // Set starting values for LDS variables
    if (WaveIsFirstLane())
    {
        tileLightsCnt = 0;
        tileDepthMin = 0x7f7fffff;
        tileDepthMax = 0;
    }

    // Create the fustrum planes
    uint2 tileCorrectScreenRes = uint2(sceneConstants.tileCorrectScreenWidth, sceneConstants.tileCorrectScreenHeight);
    
    // 1- Get screen space xleft, xright, ytop, ybottom for this tile
    uint xl = gId.x * GROUP_WIDTH;
    uint xr = (gId.x + 1) * GROUP_WIDTH;
    
    // Tiles have origin on the top left, screen space has origin on bottom left
    // So we sustract from the resolution height to correct this
    uint yb = tileCorrectScreenRes.y - ((gId.y + 1) * GROUP_WIDTH);
    uint yt = tileCorrectScreenRes.y - (gId.y * GROUP_WIDTH);
    
    // Get the view space coordinates for all 4 tile corners
    float3 topLeft     = ProjToViewSpace(ScreenToProjSpace(uint2(xl, yt), tileCorrectScreenRes)).xyz;
    float3 topRight    = ProjToViewSpace(ScreenToProjSpace(uint2(xr, yt), tileCorrectScreenRes)).xyz;
    float3 bottomRight = ProjToViewSpace(ScreenToProjSpace(uint2(xr, yb), tileCorrectScreenRes)).xyz;
    float3 bottomLeft  = ProjToViewSpace(ScreenToProjSpace(uint2(xl, yb), tileCorrectScreenRes)).xyz;
    
    GroupMemoryBarrierWithGroupSync(); // -----
    
    float3 bgWorldPos = g_texPos.Load(uint3(DTid.xy, 0));
    
    // Set depth bounds
    float4 bgViewPos = mul(float4(bgWorldPos, 1.0f), sceneConstants.view);
    uint viewDepth = asuint(bgViewPos.z);
    InterlockedMin(tileDepthMin, viewDepth);
    InterlockedMax(tileDepthMax, viewDepth);
    
    GroupMemoryBarrierWithGroupSync(); // -----

    float tileZMin = asfloat(tileDepthMin);
    float tileZMax = asfloat(tileDepthMax);
    
    uint numThreads = GROUP_WIDTH * GROUP_WIDTH;
    uint currentIndex = gTid.x + gTid.y * GROUP_WIDTH;

    while (currentIndex < sceneConstants.lightCount)
    {
        float radius = sceneConstants.lightRadius;
        float4 lightWorldPos = float4(lightcb.lightPositions[currentIndex].xyz, 1.0f);
        float4 lightPositionView = mul(lightWorldPos, sceneConstants.view);
        
        /// Cull based on depth
        uint lightViewZ = lightPositionView.z;
        if (lightViewZ + radius < tileZMin || lightViewZ - radius > tileZMax)
        {
            currentIndex += numThreads;
            continue;
        }
        
        // Cull based on the current-tile fustrum
        bool b1 = TestSphereWithFustrumPlane(topLeft, topRight, lightPositionView.xyz, radius);
        bool b2 = TestSphereWithFustrumPlane(topRight, bottomRight, lightPositionView.xyz, radius);
        bool b3 = TestSphereWithFustrumPlane(bottomRight, bottomLeft, lightPositionView.xyz, radius);
        bool b4 = TestSphereWithFustrumPlane(bottomLeft, topLeft, lightPositionView.xyz, radius);
        
        if (b1 && b2 && b3 && b4)
        {
             // Add to the counter
             uint offset;
             InterlockedAdd(tileLightsCnt, 1, offset);
             tileLightsIndices[offset] = currentIndex;
        }
        
        // Increase this to wrap if lights are bigger than numThreads
        currentIndex += numThreads;
    }
    
    GroupMemoryBarrierWithGroupSync(); // -----
    
    // Shading
    float4 albedo = g_texAlbedo.Load(uint3(DTid.xy, 0));
    float3 normal = g_texNormal.Load(uint3(DTid.xy, 0));

    // For each light in this tile, we iterate and add to the final calculation
    float3 ambient = (albedo * AMBIENT_INTENSITY * lightTints.intensity).xyz;
    float4 accum = float4(ambient + float3(lightTints.r, lightTints.g, lightTints.b), 1);
    for (int i = 0; i < tileLightsCnt; ++i)
    {
        uint lightIndex = tileLightsIndices[i];
        float radius = sceneConstants.lightRadius;
        
        // Light attenuation calculations (I think)
        float4 lightPackedData = lightcb.lightPositions[lightIndex];
        float3 lightWorldPos = lightPackedData.xyz;
        float3 lightPos = lightWorldPos  - bgWorldPos;
        float Distance = length(lightPos);
        float3 lightDir = lightPos / Distance;
        float Intensity = max(0.0f, (radius - Distance) / radius);

        uint particleIndex = (uint)lightPackedData.w;
        float3 lightColor = CACHED_COLORS[particleIndex % COLOR_COUNT].xyz;
        
        // Calculate diffuse component of directional lighting.
        float3 lighting = max(dot(normal, lightDir), 0.0f) * lightColor * Intensity;
    
        // Calculate specular component of directional lighting.
        float3 viewDir = normalize(sceneConstants.cameraPos - bgWorldPos);
        float3 halfAngle = normalize(viewDir + lightDir.xyz);
        float3 specPower = pow(saturate(dot(halfAngle, normal)), 32) * lightColor * Intensity;
    
        // Return combined lighting contributions.
        accum += float4((lighting * albedo.xyz + specPower).xyz, 0.0f);
    }
    
    uav_out[DTid.xy] = accum;
}
