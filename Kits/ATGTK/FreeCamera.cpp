//--------------------------------------------------------------------------------------
// FreeCamera.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FreeCamera.h"

using namespace DirectX;

namespace
{
    const XMVECTORF32 DEFAULT_RIGHT_VECTOR = { { { -1, 0, 0, 0 } } };
    const XMVECTORF32 DEFAULT_UP_VECTOR = { { { 0, 1, 0, 0 } } };
    const XMVECTORF32 DEFAULT_FORWARD_VECTOR = { { { 0, 0, 1, 0 } } };
}

FreeCamera::FreeCamera(float nearPlane,
    float farPlane,
    float slowSpeed,
    float fastSpeed,
    float rotateSpeed,
    bool viewMatrixAtOrigin = false) :
    mPosition(g_XMIdentityR3),
    mYaw(0),
    mPitch(0),
    mFOV(XMConvertToRadians(60.f)),
    mAspectRatio(16.0f / 9.0f),
    mNearPlane(nearPlane),
    mFarPlane(farPlane),
    mSlowSpeed(slowSpeed),
    mFastSpeed(fastSpeed),
    mRotateSpeed(rotateSpeed),
    mWorldRightVector(DEFAULT_RIGHT_VECTOR),
    mWorldUpVector(DEFAULT_UP_VECTOR),
    mWorldForwardVector(DEFAULT_FORWARD_VECTOR),
    mViewMatrixAtOrigin(viewMatrixAtOrigin)
{
}

FreeCamera::FreeCamera() : FreeCamera(1.0, 2.0, 25, 125, 2, true)
{
}

void FreeCamera::SetPosition(DirectX::XMVECTOR position)
{
    mPosition = position;
}

void FreeCamera::SetRightVector(DirectX::XMVECTOR rightVector)
{
    mWorldRightVector = rightVector;
}

void FreeCamera::SetUpVector(DirectX::XMVECTOR upVector)
{
    mWorldUpVector = upVector;
}

void FreeCamera::SetForwardVector(DirectX::XMVECTOR forwardVector)
{
    mWorldForwardVector = forwardVector;
}

void FreeCamera::Update(float dt, DirectX::GamePad::State state)
{
    float SPEED = (state.IsLeftStickPressed()) ? mFastSpeed : mSlowSpeed;

    mYaw += state.thumbSticks.rightX * dt * mRotateSpeed;
    mPitch += state.thumbSticks.rightY * dt * mRotateSpeed;

    const XMVECTOR pitchQuat = XMQuaternionRotationAxis(mWorldRightVector, mPitch);
    const XMVECTOR yawQuat = XMQuaternionRotationAxis(mWorldUpVector, -mYaw);
    const XMVECTOR combinedRotation = XMQuaternionMultiply(pitchQuat, yawQuat);

    const XMVECTOR forwardVector = XMVector3Rotate(mWorldForwardVector, combinedRotation);
    const XMVECTOR rightVector = XMVector3Rotate(mWorldRightVector, combinedRotation);
    const XMVECTOR upVector = XMVector3Rotate(mWorldUpVector, combinedRotation);

    XMVECTOR forwardOffset = XMVectorMultiply(forwardVector, XMVectorReplicate(state.thumbSticks.leftY * dt * SPEED));
    XMVECTOR rightOffset = XMVectorMultiply(rightVector, XMVectorReplicate(state.thumbSticks.leftX * dt * SPEED));
    XMVECTOR upOffset = XMVectorMultiply(mWorldUpVector, XMVectorReplicate((state.triggers.right - state.triggers.left) * dt * SPEED));

    mPosition = XMVectorAdd(mPosition, forwardOffset);
    mPosition = XMVectorAdd(mPosition, rightOffset);
    mPosition = XMVectorAdd(mPosition, upOffset);

    RecalculateInternalData(forwardVector, upVector);
}

void FreeCamera::RecalculateInternalData(DirectX::XMVECTOR forwardVector, DirectX::XMVECTOR upVector)
{
    if (mViewMatrixAtOrigin)
    {
        mViewMatrix = DirectX::XMMatrixLookToRH(XMVectorZero(), forwardVector, upVector);
    }
    else
    {
        mViewMatrix = DirectX::XMMatrixLookToRH(mPosition, forwardVector, upVector);
    }
    mProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(mFOV, mAspectRatio, mNearPlane, mFarPlane);
}
