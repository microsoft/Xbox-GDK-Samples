//--------------------------------------------------------------------------------------
// GamepadModel.cpp
//
// Displays sensor orientation on a 3D controller model.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Initialize the offscreen model renderer.
//--------------------------------------------------------------------------------------
void Sample::InitializeModelRenderer(ImGuiAtg::DeviceContext* deviceContext)
{
    m_modelViewer.SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    m_modelViewer.Initialize(deviceContext, 1280, 1024);

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring modelDir(exePath);
    modelDir = modelDir.substr(0, modelDir.find_last_of(L'\\')) + L"\\media\\Model\\";

    std::wstring meshPath = modelDir + L"XboxControllerGen3.sdkmesh";
    m_modelViewer.LoadModel(meshPath.c_str(), modelDir.c_str());
}

//--------------------------------------------------------------------------------------
// Convert the sensor quaternion to the model's coordinate system and render it.
//--------------------------------------------------------------------------------------
void Sample::RenderModel(const GameInputSensorsState& sensors)
{
    if (!m_modelViewer.IsInitialized())
        return;

    XMVECTOR sensorQuat = XMVectorSet(
        sensors.orientationX,
        sensors.orientationY,
        sensors.orientationZ,
        sensors.orientationW);

    if (XMVector4Equal(sensorQuat, XMVectorZero()))
        sensorQuat = XMQuaternionIdentity();

    XMMATRIX sensorRot = XMMatrixRotationQuaternion(sensorQuat);

    // Change basis from sensor RH Y-up coordinates to render LH Y-up coordinates.
    // Applying M * R * M^T works at every orientation, unlike remapping quaternion
    // components directly.
    XMMATRIX changeBasis = {
       -1,  0,  0, 0,
        0,  0, -1, 0,
        0, -1,  0, 0,
        0,  0,  0, 1
    };
    XMMATRIX changeBasisT = XMMatrixTranspose(changeBasis);
    XMMATRIX convertedRot = changeBasis * sensorRot * changeBasisT;

    // Align the model's rest pose with the sensor identity orientation.
    XMMATRIX baseRot = XMMatrixRotationX(XM_PI);

    constexpr float modelScale = 0.10f;
    XMMATRIX world = XMMatrixScaling(modelScale, modelScale, modelScale)
                   * baseRot
                   * convertedRot;

    m_modelViewer.Render(world);
}

//--------------------------------------------------------------------------------------
// Display the rendered model.
//--------------------------------------------------------------------------------------
void Sample::DrawModelSection(const GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("3D Orientation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (!(gamepad.deviceInfo->supportedInput & GameInputKindSensors)
            || !(gamepad.deviceInfo->sensorsInfo->supportedSensors & GameInputSensorsOrientation))
        {
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Orientation sensor not supported by this device.");
            return;
        }

        if (!m_modelViewer.IsInitialized())
        {
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "3D renderer not initialized.");
            return;
        }

        float aspect = static_cast<float>(m_modelViewer.GetRenderer().GetWidth())
                     / static_cast<float>(m_modelViewer.GetRenderer().GetHeight());
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float imageWidth = avail.x;
        float imageHeight = imageWidth / aspect;
        if (imageHeight > avail.y)
        {
            imageHeight = avail.y;
            imageWidth = imageHeight * aspect;
        }
        float offsetX = (avail.x - imageWidth) * 0.5f;
        float offsetY = (avail.y - imageHeight) * 0.5f;
        if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
        m_modelViewer.DrawImage(imageWidth, imageHeight);
    }
}