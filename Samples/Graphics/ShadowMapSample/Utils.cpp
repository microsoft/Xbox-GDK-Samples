//--------------------------------------------------------------------------------------
// Utils.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Utils.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

wchar_t const * const g_SampleTitle = L"Shadow Map Sample";
wchar_t const * const g_SampleDescription = L"Demonstrates use of various standard shadow mapping techniques.";


////////////////////////////////
///  Directional Light Info  ///
////////////////////////////////
void DirectionalLightInfo::UpdateMatrices()
{
    Vector3 f = Vector3(lightForward);
    Vector3 r = XMVector3Cross(f, Vector3(0, 1, 0));
    Vector3 u = XMVector3Cross(f, r);

    Matrix CameraRotate = Matrix(r, u, f);
    CameraRotate = CameraRotate.Invert();

    Matrix CameraTranslate = XMMatrixIdentity();
    CameraTranslate._41 = -lightPosition.x;
    CameraTranslate._42 = -lightPosition.y;
    CameraTranslate._43 = -lightPosition.z;

    dirLightView = CameraTranslate * CameraRotate;
    dirLightProj = XMMatrixOrthographicLH(lightViewportWidth, lightViewportHeight, lightNear, lightFar);
}

void DirectionalLightInfo::SetForwardVector(Vector4 forward)
{
    lightForward = forward;
    lightForward.Normalize();
}


//////////////////////
///   ShaderInfo   ///
//////////////////////
void ShaderInfo::Initialize()
{
    SetupShaderNames();
    LoadShaderBlobs();
}

void ShaderInfo::SetupShaderNames()
{
#pragma region PS Resolve
    PSNamesResolve[DEPTH_REMAPPING_VARIANCE] = L"PSResolveVariance.cso";
    PSNamesResolve[DEPTH_REMAPPING_EXPONENTIAL] = L"PSResolveExponential.cso";
    PSNamesResolve[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE] = L"PSResolveExponentialVariance.cso";
#pragma endregion

#pragma region PS Directional Blur (HORIZONTAL)
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurVarianceX1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurVarianceX1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurVarianceX2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurVarianceX2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurVarianceX4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurVarianceX4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurVarianceX6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurVarianceX6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurVarianceX8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurVarianceX8Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurExponentialX1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurExponentialX1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurExponentialX2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurExponentialX2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurExponentialX4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurExponentialX4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurExponentialX6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurExponentialX6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurExponentialX8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurExponentialX8Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurExponentialVarianceX1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurExponentialVarianceX1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurExponentialVarianceX2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurExponentialVarianceX2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurExponentialVarianceX4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurExponentialVarianceX4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurExponentialVarianceX6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurExponentialVarianceX6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurExponentialVarianceX8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_OFF][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurExponentialVarianceX8Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloat2X1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloat2X1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloat2X2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloat2X2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloat2X4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloat2X4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloat2X6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloat2X6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloat2X8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloat2X8Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloatX1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloatX1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloatX2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloatX2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloatX4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloatX4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloatX6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloatX6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloatX8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloatX8Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloat4X1Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloat4X1Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloat4X2Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloat4X2Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloat4X4Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloat4X4Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloat4X6Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloat4X6Square.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloat4X8Gaussian.cso";
    DirectionalBlurHrzPSNames[MSAA_ON][DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloat4X8Square.cso";
#pragma endregion

#pragma region PS Directional Blur (VERTICAL, 2d BLUR PASS)
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloat2Y1Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloat2Y1Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloat2Y2Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloat2Y2Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloat2Y4Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloat2Y4Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloat2Y6Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloat2Y6Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloat2Y8Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloat2Y8Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloatY1Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloatY1Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloatY2Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloatY2Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloatY4Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloatY4Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloatY6Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloatY6Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloatY8Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloatY8Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][0] = L"PSDirectionalBlurFloat4Y1Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_1x1][1] = L"PSDirectionalBlurFloat4Y1Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][0] = L"PSDirectionalBlurFloat4Y2Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_2x2][1] = L"PSDirectionalBlurFloat4Y2Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][0] = L"PSDirectionalBlurFloat4Y4Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_4x4][1] = L"PSDirectionalBlurFloat4Y4Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][0] = L"PSDirectionalBlurFloat4Y6Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_6x6][1] = L"PSDirectionalBlurFloat4Y6Square.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][0] = L"PSDirectionalBlurFloat4Y8Gaussian.cso";
    DirectionalBlurVrtPSNames[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE][BLUR_KERNEL_8x8][1] = L"PSDirectionalBlurFloat4Y8Square.cso";
#pragma endregion

#pragma region PS Lit With shadows
    ShadingPCFGatherCmp[BLUR_KERNEL_4x4 - 1][0] = L"PSLitWithShadowPCF4x4GatherCmpGaussian.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_2x2 - 1][0] = L"PSLitWithShadowPCF2x2GatherCmpGaussian.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_2x2 - 1][1] = L"PSLitWithShadowPCF2x2GatherCmpSquare.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_4x4 - 1][1] = L"PSLitWithShadowPCF4x4GatherCmpSquare.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_6x6 - 1][0] = L"PSLitWithShadowPCF6x6GatherCmpGaussian.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_6x6 - 1][1] = L"PSLitWithShadowPCF6x6GatherCmpSquare.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_8x8 - 1][0] = L"PSLitWithShadowPCF8x8GatherCmpGaussian.cso";
    ShadingPCFGatherCmp[BLUR_KERNEL_8x8 - 1][1] = L"PSLitWithShadowPCF8x8GatherCmpSquare.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_2x2 - 1][0] = L"PSLitWithShadowPCF2x2SampleCmpStep1Gaussian.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_2x2 - 1][1] = L"PSLitWithShadowPCF2x2SampleCmpStep1Square.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_4x4 - 1][0] = L"PSLitWithShadowPCF4x4SampleCmpStep1Gaussian.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_4x4 - 1][1] = L"PSLitWithShadowPCF4x4SampleCmpStep1Square.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_6x6 - 1][0] = L"PSLitWithShadowPCF6x6SampleCmpStep1Gaussian.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_6x6 - 1][1] = L"PSLitWithShadowPCF6x6SampleCmpStep1Square.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_8x8 - 1][0] = L"PSLitWithShadowPCF8x8SampleCmpStep1Gaussian.cso";
    ShadingPCFSampleCmpStep1[BLUR_KERNEL_8x8 - 1][1] = L"PSLitWithShadowPCF8x8SampleCmpStep1Square.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_2x2 - 1][0] = L"PSLitWithShadowPCF2x2SampleCmpStep2Gaussian.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_2x2 - 1][1] = L"PSLitWithShadowPCF2x2SampleCmpStep2Square.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_4x4 - 1][0] = L"PSLitWithShadowPCF4x4SampleCmpStep2Gaussian.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_4x4 - 1][1] = L"PSLitWithShadowPCF4x4SampleCmpStep2Square.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_6x6 - 1][0] = L"PSLitWithShadowPCF6x6SampleCmpStep2Gaussian.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_6x6 - 1][1] = L"PSLitWithShadowPCF6x6SampleCmpStep2Square.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_8x8 - 1][0] = L"PSLitWithShadowPCF8x8SampleCmpStep2Gaussian.cso";
    ShadingPCFSampleCmpStep2[BLUR_KERNEL_8x8 - 1][1] = L"PSLitWithShadowPCF8x8SampleCmpStep2Square.cso";
    ShadingNonPCF[DEPTH_REMAPPING_VARIANCE] = L"PSLitWithShadowVariance.cso";
    ShadingNonPCF[DEPTH_REMAPPING_EXPONENTIAL] = L"PSLitWithShadowExponential.cso";
    ShadingNonPCF[DEPTH_REMAPPING_EXPONENTIAL_VARIANCE] = L"PSLitWithShadowExponentialVariance.cso";
    ShadingNonPCF[DEPTH_REMAPPING_STANDARD] = L"PSLitWithShadowPoint.cso";
#pragma endregion
}

void ShaderInfo::LoadShaderBlobs()
{
    // Individual BLOBS we want to keep around
    VS_BLOB = DX::ReadData(VShaderName);
    VSNull_BLOB = DX::ReadData(VShaderNullName);
    GS_BLOB = DX::ReadData(GShaderName);
    PS_Unlit_BLOB = DX::ReadData(PSUnlitName);
    PS_LightSrc_BLOB = DX::ReadData(PSLightSrcName);

    // Loading BLOBS for Resolve
    for (size_t i = 0; i < DEPTH_REMAPPING_COUNT - 1; ++i)
    {
        auto shaderName = PSNamesResolve[i];
        Resolve_BLOBS[i] = DX::ReadData(shaderName);
    }

    // Loading BLOBS for horizontal blurring pass
    for (size_t i = 0; i < MSAA_ON_OFF_COUNT; ++i) // we are counting msaa as a binary option, not the msaa multiple values
    {
        for (size_t j = 0; j < DEPTH_REMAPPING_COUNT - 1; ++j)
        {
            for (size_t k = 0; k < BLUR_KERNEL_COUNT; ++k)
            {
                for (size_t l = 0; l < BLUR_TYPE_COUNT; ++l)
                {
                    auto shaderName = DirectionalBlurHrzPSNames[i][j][k][l];
                    FilterHrz_BLOBS[i][j][k][l] = DX::ReadData(shaderName);
                }
            }
        }
    }

    // Loading BLOBS for vertical blurring pass
    for (size_t j = 0; j < DEPTH_REMAPPING_COUNT - 1; ++j)
    {
        for (size_t k = 0; k < BLUR_KERNEL_COUNT; ++k)
        {
            for (size_t l = 0; l < BLUR_TYPE_COUNT; ++l)
            {
                auto shaderName = DirectionalBlurVrtPSNames[j][k][l];
                FilterVrt_BLOBS[j][k][l] = DX::ReadData(shaderName);
            }
        }
    }

    // Loading BLOBS for PCFSampleCmpStep1 and PCFSampleCmpStep2 and PCFGatherCmp
    for (size_t i = 0; i < BLUR_KERNEL_COUNT - 1; ++i)
    {
        for (size_t j = 0; j < BLUR_TYPE_COUNT; ++j)
        {
            auto shaderNameStep1 = ShadingPCFSampleCmpStep1[i][j];
            ShadingPCFSampleCmpStep1_BLOBS[i][j] = DX::ReadData(shaderNameStep1);

            auto shaderNameStep2 = ShadingPCFSampleCmpStep2[i][j];
            ShadingPCFSampleCmpStep2_BLOBS[i][j] = DX::ReadData(shaderNameStep2);

            auto shaderNameGather = ShadingPCFGatherCmp[i][j];
            ShadingPCFGatherCmp_BLOBS[i][j] = DX::ReadData(shaderNameGather);
        }
    }

    // Loading BLOBS for ShadingNonPCF
    for (size_t i = 0; i < DEPTH_REMAPPING_COUNT; ++i)
    {
        auto shaderName = ShadingNonPCF[i];
        ShadingNoPCF_BLOBS[i] = DX::ReadData(shaderName);
    }
}
