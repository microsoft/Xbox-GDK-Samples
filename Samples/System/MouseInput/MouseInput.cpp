//--------------------------------------------------------------------------------------
// MouseInput.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MouseInput.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_RotationGain = 0.004f; // sensitivity adjustment
}

Sample::Sample() noexcept(false) :
    m_leftButton(false),
    m_rightButton(false),
    m_frame(0),
    m_isAbsolute(true),
    m_isRelative(false),
    m_isEdgeMove(false),
    m_highlightFPS(false),
    m_highlightRTS(false),
    m_mouseDown(false),
    m_eyeFPS(0.f, 20.f, -20.f),
    m_targetFPS(0.f, 20.f, 0.f),
    m_eyeRTS(0.f, 300.f, 0.f),
    m_targetRTS(0.01f, 300.1f, 0.01f),
    m_eye(0.f, 20.f, 0.f),
    m_target(0.01f, 20.1f, 0.01f),
    m_pitch(0),
    m_yaw(0),
    m_fullscreenRect{},
    m_FPStile{},
    m_RTStile{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    // For PresentX presentation loops, the wait for the origin event
    // should be just before input is processed.
    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    if (SUCCEEDED(m_gameInput->GetCurrentReading(GameInputKindGamepad | GameInputKindMouse, nullptr, &m_reading)))
    {
        GameInputGamepadState gamepadState;

        if (m_reading->GetGamepadState(&gamepadState))
        {
            if (gamepadState.buttons & GameInputGamepadView)
            {
                ExitSample();
            }

            if (gamepadState.buttons & GameInputGamepadDPadLeft)
            {
                SetMode(RELATIVE_MOUSE);
            }

            if (gamepadState.buttons & GameInputGamepadDPadRight)
            {
                SetMode(EDGECURSOR_MOUSE);
            }
        }

        GameInputMouseState mouseState;
        if (m_reading->GetMouseState(&mouseState))
        {
            if (m_isAbsolute)
            {
                if (m_mouseDown)
                {
                    if (mouseState.buttons == GameInputMouseNone)
                    {
                        m_mouseDown = false;
                    }
                }
                else
                {
                    //Select mode
                    if (mouseState.buttons == GameInputMouseLeftButton)
                    {
                        SetMode(RELATIVE_MOUSE);
                    }
                    else if (mouseState.buttons == GameInputMouseRightButton)
                    {
                        SetMode(EDGECURSOR_MOUSE);
                    }
                }
            }
            else if (m_isEdgeMove)
            {
                if (m_leftButton && m_rightButton)
                {
                    m_mouseDown = true;
                    SetMode(ABSOLUTE_MOUSE);
                }

                //See WndProc in Main.cpp for mouse cursor and button updates for this mode
            }
            else if (m_isRelative)
            {
                if (mouseState.buttons == (GameInputMouseLeftButton | GameInputMouseRightButton))
                {
                    m_mouseDown = true;
                    SetMode(ABSOLUTE_MOUSE);
                }
                else
                {
                    UpdateCamera(Vector3((float)mouseState.positionX - m_lastMouseState.positionX, (float)mouseState.positionY - m_lastMouseState.positionY, 0.f));
                }
            }

            m_lastMouseState = mouseState;
        }
    }

    // In edge cursor mode implement screen scrolling when the mouse is near the edge of the screen
    if (m_isEdgeMove)
    {
        if (m_screenLocation_x < 20.f)
            MoveRight(-25.f);
        else if (m_screenLocation_x > m_deviceResources->GetOutputSize().right - m_deviceResources->GetOutputSize().left - 20.f)
            MoveRight(25.f);

        if (m_screenLocation_y < 20.f)
            MoveForward(25.f);
        else if (m_screenLocation_y > m_deviceResources->GetOutputSize().bottom - m_deviceResources->GetOutputSize().top - 20.f)
            MoveForward(-25.f);
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    auto output = m_deviceResources->GetOutputSize();

    if (m_isRelative)
    {
        ID3D12DescriptorHeap* heaps[] = { m_modelFPSResources->Heap(), m_states->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        Model::UpdateEffectMatrices(m_modelFPSEffect, m_world, m_view, m_proj);

        m_modelFPS->Draw(commandList, m_modelFPSEffect.cbegin());
    }
    else if (m_isEdgeMove)
    {
        ID3D12DescriptorHeap* heaps[] = { m_modelRTSResources->Heap(), m_states->Heap() };
        commandList->SetDescriptorHeaps(_countof(heaps), heaps);

        Model::UpdateEffectMatrices(m_modelRTSEffect, m_world, m_view, m_proj);

        m_modelRTS->Draw(commandList, m_modelRTSEffect.cbegin());
    }
    
    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    std::wstring text = L"+";

    const wchar_t* outputString = text.c_str();
    Vector2 origin = Vector2(m_font->MeasureString(outputString)) / 2.f;

    m_fontPos.x = (float)m_screenLocation_x;
    m_fontPos.y = (float)m_screenLocation_y;

    if (m_isAbsolute)
    {
        m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), output);
        m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Menu), XMUINT2(320, 240), m_FPStile);
        m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Menu), XMUINT2(320, 240), m_RTStile);

        if (m_highlightFPS)
        {
            m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::MenuHighlight), XMUINT2(320, 240), m_FPStile);
        }
        else if (m_highlightRTS)
        {
            m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::MenuHighlight), XMUINT2(320, 240), m_RTStile);
        }

        std::wstring text1 = L"Mouse Cursor Sample: ";

        const wchar_t* outputTitle = text1.c_str();
        Vector2 originTitle = Vector2(m_font64->MeasureString(outputTitle)) / 2.f;
        std::wstring text2 = L"Choose a game mode";
        const wchar_t* outputSubtitle = text2.c_str();
        Vector2 originSubtitle = Vector2(m_font32->MeasureString(outputSubtitle)) / 2.f;

        std::wstring text3 = L"First-person Shooter \n\nLeft mouse button";
        const wchar_t* outputFPS = text3.c_str();
        Vector2 originFPS = Vector2(m_font28->MeasureString(outputFPS)) / 2.f;
        std::wstring text4 = L"Real-time Strategy \n\nRight mouse button";
        const wchar_t* outputRTS = text4.c_str();
        Vector2 originRTS = Vector2(m_font28->MeasureString(outputRTS)) / 2.f;

        m_font64->DrawString(m_batch.get(), outputTitle, m_fontPosTitle, Colors::White, 0.f, originTitle);
        m_font32->DrawString(m_batch.get(), outputSubtitle, m_fontPosSubtitle, Colors::White, 0.f, originSubtitle);
        m_font28->DrawString(m_batch.get(), outputFPS, m_fontPosFPS, Colors::White, 0.f, originFPS);
        m_font28->DrawString(m_batch.get(), outputRTS, m_fontPosRTS, Colors::White, 0.f, originRTS);
    }
    else
    {
        m_font->DrawString(m_batch.get(), outputString, m_fontPos, Colors::White, 0.f, origin);
    }

    m_batch->End();
    
    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    SetMode(ABSOLUTE_MOUSE);
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_states = std::make_unique<CommonStates>(device);

    m_modelFPS = Model::CreateFromSDKMESH(device, L"FPSRoom.sdkmesh");
    m_modelRTS = Model::CreateFromSDKMESH(device, L"3DRTSMap.sdkmesh");

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"Courier_36.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont36),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont36));

    m_font64 = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_34.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont34),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont34));

    m_font32 = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_24.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont24),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont24));

    m_font28 = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_22.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont22),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont22));

    m_deviceResources->WaitForGpu();

    m_modelFPS->LoadStaticBuffers(device, upload);
    m_modelFPSResources = m_modelFPS->LoadTextures(device, upload);
    m_FPSfxFactory = std::make_unique<EffectFactory>(m_modelFPSResources->Heap(), m_states->Heap());

    m_modelRTSResources = std::make_unique<DX::CompressedTextureFactory>(device, upload, m_modelRTS->textureNames.size());
    m_modelRTS->LoadTextures(*m_modelRTSResources);
    m_modelRTS->LoadStaticBuffers(device, upload);
    m_RTSfxFactory = std::make_unique<EffectFactory>(m_modelRTSResources->Heap(), m_states->Heap());

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload,
            L"Assets//background_flat.png",
            m_texture_background.ReleaseAndGetAddressOf())
    );
    
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload,
            L"Assets//green_tile.png",
            m_texture_tile.ReleaseAndGetAddressOf())
    );

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload,
            L"Assets//green_tile_border.png",
            m_texture_tile_border.ReleaseAndGetAddressOf())
    );

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    CreateShaderResourceView(device, m_texture_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
    CreateShaderResourceView(device, m_texture_tile.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Menu));
    CreateShaderResourceView(device, m_texture_tile_border.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::MenuHighlight));

    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        EffectPipelineStateDescription pdAlpha(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        m_modelFPSEffect = m_modelFPS->CreateEffects(*m_FPSfxFactory, pd, pdAlpha);
        m_modelRTSEffect = m_modelRTS->CreateEffects(*m_RTSfxFactory, pd, pdAlpha);
    }

    m_world = Matrix::Identity;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);

    // Initialization of background image
    m_fullscreenRect = m_deviceResources->GetOutputSize();

    int backBufferWidth = static_cast<int>(m_fullscreenRect.right - m_fullscreenRect.left);
    int backBufferHeight = static_cast<int>(m_fullscreenRect.bottom - m_fullscreenRect.top);

    // Initialize UI tiles and font locations
    m_FPStile.left = (LONG)(0.325f*backBufferWidth);
    m_FPStile.top = (LONG)(0.44f*backBufferHeight);
    m_FPStile.right = (LONG)(0.495f*backBufferWidth);
    m_FPStile.bottom = (LONG)(0.66f*backBufferHeight);
    m_FPStile.bottom = std::max(m_FPStile.bottom, m_FPStile.top + 150);
    m_FPStile.left = std::min(m_FPStile.left, (LONG)(m_FPStile.right - ((m_FPStile.bottom - m_FPStile.top) * 4 / 3.f)));

    m_RTStile.left = (LONG)(0.505f*backBufferWidth);
    m_RTStile.top = m_FPStile.top;
    m_RTStile.right = (LONG)(0.675f*backBufferWidth);
    m_RTStile.bottom = m_FPStile.bottom;
    m_RTStile.right = std::max(m_RTStile.right, (LONG)(m_RTStile.left + ((m_RTStile.bottom - m_RTStile.top) * 4 / 3.f)));

    m_fontPos.x = backBufferWidth / 2.f;
    m_fontPos.y = backBufferHeight / 2.f;

    m_fontPosTitle.x = backBufferWidth / 2.f;
    m_fontPosTitle.y = backBufferHeight * 0.27f;

    m_fontPosSubtitle.x = backBufferWidth / 2.f;
    m_fontPosSubtitle.y = backBufferHeight * 0.36f;

    m_fontPosFPS.x = m_FPStile.left + (m_FPStile.right - m_FPStile.left) / 2.f;
    m_fontPosFPS.y = m_FPStile.top + (m_FPStile.bottom - m_FPStile.top) / 2.f;

    m_fontPosRTS.x = m_RTStile.left + (m_RTStile.right - m_RTStile.left) / 2.f;
    m_fontPosRTS.y = m_fontPosFPS.y;
}
#pragma endregion


// Change the target value based on the mouse movement for move-look/relative mouse mode
void Sample::UpdateCamera(Vector3 movement)
{
    // Adjust pitch and yaw based on the mouse movement
    Vector3 rotationDelta = movement * c_RotationGain;
    m_pitch += rotationDelta.y;
    m_yaw += rotationDelta.x;

    // Limit to avoid looking directly up or down
    const float limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-limit, std::min(+limit, m_pitch));

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    float y = sinf(m_pitch);
    float r = cosf(m_pitch);
    float z = r * cosf(m_yaw);
    float x = r * sinf(m_yaw);

    m_target = m_eye + Vector3(x, y, z);

    SetView();
}

// Move the camera forward or backward
void Sample::MoveForward(float amount)
{
    Vector3 movement = m_target - m_eye;
    movement.y = 0.f;
    Vector3 m_eye_temp = m_eye - amount * movement;

    if ((m_eye_temp.z < -1 * m_eye_temp.x + 400) && (m_eye_temp.z < m_eye_temp.x + 800)
        && (m_eye_temp.z > -1 * m_eye_temp.x - 300) && (m_eye_temp.z > m_eye_temp.x - 800))
    {
        m_eye = m_eye_temp;
        m_target = m_target - amount * movement;

        SetView();
    }
}

// Move the camera to the right or left 
void Sample::MoveRight(float amount)
{
    Vector3 movement1 = m_target - m_eye;
    movement1.y = 0.f;
    Vector3 movement;
    movement.x = -movement1.z;
    movement.y = 0.f;
    movement.z = movement1.x;


    Vector3 m_eye_temp = m_eye + amount * movement;

    if ((m_eye_temp.z < (-1 * m_eye_temp.x + 400)) && (m_eye_temp.z < (m_eye_temp.x + 800))
        && (m_eye_temp.z > (-1 * m_eye_temp.x - 300)) && (m_eye_temp.z > (m_eye_temp.x - 800)))
    {
        m_eye = m_eye_temp;
        m_target = m_target + amount * movement;

        SetView();
    }
}

// Update the viewport based on the updated eye and target values
void Sample::SetView()
{
    UINT backBufferWidth = static_cast<UINT>(m_deviceResources->GetOutputSize().right - m_deviceResources->GetOutputSize().left);
    UINT backBufferHeight = static_cast<UINT>(m_deviceResources->GetOutputSize().bottom - m_deviceResources->GetOutputSize().top);

    m_view = Matrix::CreateLookAt(m_eye, m_target, Vector3::UnitY);
    m_proj = XMMatrixPerspectiveFovLH(XM_PI / 4.f, float(backBufferWidth) / float(backBufferHeight), 0.1f, 10000.f);
}

// Set mode to relative ( 1 ), absolute ( 0 ), or edge cursor ( 2 )
void Sample::SetMode(MouseMode newMode)
{
    // If entering FPS or relative mode
    if (newMode == RELATIVE_MOUSE)
    {
        m_isRelative = true;
        m_isAbsolute = false;
        m_isEdgeMove = false;
        m_highlightFPS = false;
        m_highlightRTS = false;
        m_world = Matrix::CreateRotationX(XM_PI / 2.f)*Matrix::CreateRotationY(XM_PI);
        m_eye = m_eyeFPS;
        m_target = m_targetFPS;
        UpdateCamera(Vector3::Zero);
        SetView();
    }
    // If entering RTS or edge cursor mode
    else if (newMode == EDGECURSOR_MOUSE)
    {
        m_isRelative = false;
        m_isAbsolute = false;
        m_isEdgeMove = true;
        m_highlightFPS = false;
        m_highlightRTS = false;
        m_world = Matrix::CreateRotationX(XM_PI / 2.f)*Matrix::CreateRotationY(5 * XM_PI / 4);
        m_eye = m_eyeRTS;
        m_target = m_targetRTS;
        SetView();
    }
    // Entering absolute mode
    else
    {
        if (m_isEdgeMove)
        {
            m_eyeRTS = m_eye;
            m_targetRTS = m_target;
        }
        m_isRelative = false;
        m_isAbsolute = true;
        m_isEdgeMove = false;
    }
}
