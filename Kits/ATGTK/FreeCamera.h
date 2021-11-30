//--------------------------------------------------------------------------------------
// FreeCamera.h
//
// Free-look camera with controller support (no mouse/keyboard).
// Maintains consistent up-direction, like an FPS-camera.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class FreeCamera
{
public:
    FreeCamera(float nearPlane,
        float farPlane,
        float slowSpeed,
        float fastSpeed,
        float rotateSpeed,
        bool viewMatrixAtOrigin);
    FreeCamera();

    void SetPosition(DirectX::XMVECTOR position);
    void SetRightVector(DirectX::XMVECTOR rightVector);
    void SetUpVector(DirectX::XMVECTOR upVector);
    void SetForwardVector(DirectX::XMVECTOR forwardVector);
    void SetYaw(float yaw) { mYaw = yaw; }
    void SetAspectRatio(float ratio) { mAspectRatio = ratio; }
    void Update(float dt, DirectX::GamePad::State state);

    DirectX::XMVECTOR GetPosition() { return mPosition; }
    DirectX::XMMATRIX GetViewMatrix() { return mViewMatrix; }
    DirectX::XMMATRIX GetProjectionMatrix() { return mProjectionMatrix; }

private:

    void RecalculateInternalData(DirectX::XMVECTOR forwardVector, DirectX::XMVECTOR upVector);

private:
    DirectX::XMVECTOR mPosition;

    float mYaw;
    float mPitch;
    float mFOV;
    float mAspectRatio;

    float mNearPlane;
    float mFarPlane;

    float mSlowSpeed;
    float mFastSpeed;
    float mRotateSpeed;

    DirectX::XMMATRIX mViewMatrix;
    DirectX::XMMATRIX mProjectionMatrix;

    DirectX::XMVECTOR mWorldRightVector;
    DirectX::XMVECTOR mWorldUpVector;
    DirectX::XMVECTOR mWorldForwardVector;

    bool mViewMatrixAtOrigin;
};

