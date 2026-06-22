//--------------------------------------------------------------------------------------
// ConstantBuffer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus 
using float4x4 = DirectX::XMFLOAT4X4;
using float3x3 = DirectX::XMFLOAT3X3;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using uint = uint32_t;
#endif

#define MIPS_IN_ONE_SHADER 4

struct Constants
{
    float4x4 WorldViewProj;
    float4x4 WorldInverseTranspose;
    float aoMultiplier;
};

struct SSAOConstants
{
    float ZMagic;
    float ZClear;
    uint HTileInfo;
    uint FrameIndex;
};

struct BlurAndUpsampleConstants
{
    float2 InvLowResolution;
    float2 InvHighResolution;
    float NoiseFilterStrength;
    float StepSize;
    float kBlurTolerance;
    float kUpsampleTolerance;
};

struct SSOARenderConstants
{
    float4 gInvThicknessTable[3];
    float4 gSampleWeightTable[3];
    float2 gInvSliceDimension;
    float  gRejectFadeoff;
    float  gRcpAccentuation;
};
