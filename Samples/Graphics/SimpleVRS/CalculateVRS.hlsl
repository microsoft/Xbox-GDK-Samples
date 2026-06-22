//--------------------------------------------------------------------------------------
// CalculateVRS.hlsl
//
// A shader that populates the VRS Shading Rate Image from an input texture with transparency values.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Lifted from the spec https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
#define D3D12_SHADING_RATE_1X1 0x0      // No change to shading rate
#define D3D12_SHADING_RATE_1X2 0x1      // Reduces vertical resolution 2x
#define D3D12_SHADING_RATE_2X1 0x4      // Reduces horizontal resolution 2x
#define D3D12_SHADING_RATE_2X2 0x5      // Reduces both axes by 2x

#define m_rootSignature \
	"DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
	"CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
	"DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\


cbuffer VRSCB : register(b0)
{
    float2      shadingRateImageDimensions;
};

RWTexture2D<uint> ShadingRateImage     : register(u0);
Texture2D<float> TransparencyTexture   : register(t0);

// Shading rates are naively assigned based on alpha value gradients
// of neighbouring pixel for a given value of E. 
// The insides of clouds in the transparency texture are shaded with 2x2,
// while the outlines and other prominent edges are appropriately
// shaded with 1x2 or 2x1 (vertical VS horizontal edges). 

//|------|------|------|
//|  A   |  B   |  C   |
//|------|------|------|
//|  D   |  E   |  F   |
//|------|------|------|
//|  G   |  H   |  I   |
//|------|------|------|

[RootSignature(m_rootSignature)]
[numthreads(8, 8, 1)]
void main(uint2 groupThreadId : SV_GroupThreadId, uint2 groupId : SV_GroupID)
{
    uint2 threadCoord = groupId.xy * 8 + groupThreadId.xy;
    uint tileShadingRate = D3D12_SHADING_RATE_1X1; // Default shading rate if no part of the cloud covers the scene.
    float E = TransparencyTexture[threadCoord];

    // Assign VRS rates based on number of similar pixels in the X and Y directions.
    // Assumes that pixels processed aren't fully transparent.
    if (E > 0.0f)
    {
        // Any spaces left behind by the other shading rates will be filled in with the shading rate 
        // that has the widest coverage.
        tileShadingRate = D3D12_SHADING_RATE_2X2;

        // The maximum difference in alpha values between neighbouring pixels to be considered similar
        float threshhold = 0.02f;

        float B = TransparencyTexture[threadCoord + uint2(0, -1)];
        float D = TransparencyTexture[threadCoord + uint2(-1, 0)];
        float F = TransparencyTexture[threadCoord + uint2(1, 0)];
        float H = TransparencyTexture[threadCoord + uint2(0, 1)];

        // Assign alpha value of 0 to neighbours of border pixels.
        if (threadCoord.x == 0.0f) D = 0.0f;
        if (threadCoord.x == (int)shadingRateImageDimensions.x - 1) F = 0.0f;
        if (threadCoord.y == 0.0f) B = 0;
        if (threadCoord.y == (int)shadingRateImageDimensions.y - 1) H = 0.0f;

        float left = abs(D - E);
        float right = abs(F - E);
        float up = abs(B - E);
        float down = abs(H - E);
        int xRateVotes = 0;
        int yRateVotes = 0;

        // Horizontal edges
        if (left < threshhold) xRateVotes = xRateVotes + 1;
        if (right < threshhold) xRateVotes = xRateVotes + 1;

        // Vertical edges
        if (up < threshhold) yRateVotes = yRateVotes + 1;
        if (down < threshhold) yRateVotes = yRateVotes + 1;

        // Shading rate assignment for defining edges
        if (xRateVotes > yRateVotes) tileShadingRate = D3D12_SHADING_RATE_2X1;
        if (yRateVotes > xRateVotes) tileShadingRate = D3D12_SHADING_RATE_1X2;

        // For pixels on contours
        if (B == 0.0f || H == 0.0f) tileShadingRate = D3D12_SHADING_RATE_2X1;
        if (D == 0.0f || F == 0.0f) tileShadingRate = D3D12_SHADING_RATE_1X2;
    }
    ShadingRateImage[threadCoord.xy] = tileShadingRate;
}
