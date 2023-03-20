//--------------------------------------------------------------------------------------
// CloudVariableReplacement.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CloudVariableReplacement.h"
#include <inttypes.h>

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    void CALLBACK ConnectionStateChangedCallback(
        void* context,
        XGameStreamingClientId client,
        XGameStreamingConnectionState state) noexcept
    {
        auto sample = reinterpret_cast<Sample*>(context);

        if (state == XGameStreamingConnectionState::Connected)
        {
            for (size_t i = 0; i < c_maxClients; ++i)
            {
                if (sample->m_clients[i].id == XGameStreamingNullClientId)
                {
                    sample->m_clients[i].id = client;

                    //Check to see if this client has a small display
                    uint32_t clientWidthMm = 0;
                    uint32_t clientHeightMm = 0;
                    if (SUCCEEDED(XGameStreamingGetStreamPhysicalDimensions(sample->m_clients[i].id, &clientWidthMm, &clientHeightMm)))
                    {
                        if (clientWidthMm * clientHeightMm < 13000)
                        {
                            sample->m_clients[i].smallScreen = true;
                        }
                    }

                    //The sample TAK is 10.0, so ensure that the client has the overlay
                    XVersion overlayVersion;
                    if (SUCCEEDED(XGameStreamingGetTouchBundleVersion(sample->m_clients[i].id, &overlayVersion, 0, nullptr)))
                    {
                        if (overlayVersion.major == 10 && overlayVersion.minor == 0)
                        {
                            sample->m_clients[i].validOverlay = true;
                        }
                    }

                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < c_maxClients; ++i)
            {
                if (sample->m_clients[i].id == client)
                {
                    sample->m_clients[i] = ClientDevice();
                }
            }
        }

        sample->UpdateClientState();
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_buttonDown(false),
    m_streaming(false),
    m_validOverlay(false),
    m_showStandard(true),
    m_yVisibility(true),
    m_aEnabled(true),
    m_bOpacity(1.f)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

Sample::~Sample()
{
    XGameStreamingUnregisterConnectionStateChanged(m_token, false);
    XGameStreamingUninitialize();
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    DX::ThrowIfFailed(GameInputCreate(&m_gameInput));

    m_font = m_localFont.get();

    DX::ThrowIfFailed(XTaskQueueCreate(XTaskQueueDispatchMode::Immediate, XTaskQueueDispatchMode::Immediate, &m_queue));

    DX::ThrowIfFailed(XGameStreamingInitialize());
    DX::ThrowIfFailed(XGameStreamingRegisterConnectionStateChanged(m_queue, this, ConnectionStateChangedCallback, &m_token));
    XGameStreamingShowTouchControlLayout("standard-variable-replacement");
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %I64u", m_frame);

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

    if (XGameStreamingIsStreaming() != m_streaming)
    {
        //The streaming state has changed
        m_streaming = !m_streaming;
    }

    //Get only gamepad and touch input types
    HRESULT hr = m_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &m_reading);

    if (SUCCEEDED(hr))
    {
        GameInputGamepadState gamepadState;

        if (m_reading->GetGamepadState(&gamepadState))
        {
            bool needUpdate = false;

            if (gamepadState.buttons & GameInputGamepadDPadUp && !m_buttonDown)
            {
                m_buttonDown = true;
                needUpdate = true;
                m_yVisibility = !m_yVisibility;
            }
            else if (gamepadState.buttons & GameInputGamepadDPadDown && !m_buttonDown)
            {
                m_buttonDown = true;
                needUpdate = true;
                m_aEnabled = !m_aEnabled;
            }
            else if (gamepadState.buttons & GameInputGamepadDPadRight && !m_buttonDown)
            {
                m_buttonDown = true;
                needUpdate = true;
                m_bOpacity = std::min(m_bOpacity + .1f, 1.);
            }
            else if (gamepadState.buttons & GameInputGamepadDPadLeft && !m_buttonDown)
            {
                m_buttonDown = true;
                needUpdate = true;
                m_aEnabled = !m_aEnabled;
                m_bOpacity = std::max(m_bOpacity - .1f, 0.);
            }
            else if (gamepadState.buttons & GameInputGamepadRightShoulder && !m_buttonDown)
            {
                m_buttonDown = true;
                needUpdate = true;
                m_showStandard = !m_showStandard;

                if (m_showStandard)
                {
                    XGameStreamingShowTouchControlLayout("standard-variable-replacement");
                }
                else
                {
                    XGameStreamingShowTouchControlLayout("fighting-variable-replacement");
                }
            }
            else if (m_buttonDown && !(gamepadState.buttons & GameInputGamepadDPadUp
                || gamepadState.buttons & GameInputGamepadDPadDown
                || gamepadState.buttons & GameInputGamepadDPadLeft
                || gamepadState.buttons & GameInputGamepadDPadRight
                || gamepadState.buttons & GameInputGamepadRightShoulder))
            {
                m_buttonDown = false;
            }

            if (needUpdate)
            {
                UpdateOverlayState();
            }
        }
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

    auto fullscreen = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    if (!m_streaming)
    {
        m_font->DrawString(m_batch.get(), L"Game is not currently being streamed", pos, ATG::Colors::Orange);
    }
    else if (!m_validOverlay)
    {
        m_font->DrawString(m_batch.get(), L"The overlay is not loaded on a streaming client. Please view the Readme.", pos, ATG::Colors::Orange);
    }
    else
    {
        DX::DrawControllerString(m_batch.get(), m_font, m_ctrlFont.get(), L"[DPad]Left: Reduce opacity of [B]", pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
        DX::DrawControllerString(m_batch.get(), m_font, m_ctrlFont.get(), L"[DPad]Right: Increase opacity of [B]", pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
        DX::DrawControllerString(m_batch.get(), m_font, m_ctrlFont.get(), L"[DPad]Up: Toggle visibility of [Y]", pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
        DX::DrawControllerString(m_batch.get(), m_font, m_ctrlFont.get(), L"[DPad]Down: Toggle [A] enabled", pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
        DX::DrawControllerString(m_batch.get(), m_font, m_ctrlFont.get(), L"[RB]: Toggle layout", pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_localFont = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_24.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintLocalFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintLocalFont));

    m_remoteFont = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_36.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintRemoteFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintRemoteFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"callout_circle.dds", m_circleTexture.ReleaseAndGetAddressOf()));
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_circleTexture.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Touch));
    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}

//--------------------------------------------------------------------------------------
// Name: UpdateClientState()
// Desc: Updates overlay and screen size state for clients
//--------------------------------------------------------------------------------------
void Sample::UpdateClientState()
{
    m_font = m_localFont.get();
    m_validOverlay = true;

    for (int i = 0; i < c_maxClients; i++)
    {
        if (m_clients[i].id != XGameStreamingNullClientId)
        {
            if (m_clients[i].smallScreen)
            {
                m_font = m_remoteFont.get();
            }

            if (!m_clients[i].validOverlay)
            {
                m_validOverlay = false;
            }
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: UpdateOverlayState()
// Desc: Updates overlay state to match input
//--------------------------------------------------------------------------------------
void Sample::UpdateOverlayState()
{
    XGameStreamingTouchControlsStateOperation stateOps[3];

    //Y visibility
    stateOps[0].path = "/Y_Visibility";
    stateOps[0].operationKind = XGameStreamingTouchControlsStateOperationKind::Replace;
    stateOps[0].value.valueKind = XGameStreamingTouchControlsStateValueKind::Boolean;
    stateOps[0].value.booleanValue = m_yVisibility;

    //A enabled
    stateOps[1].path = "/A_Enabled";
    stateOps[1].operationKind = XGameStreamingTouchControlsStateOperationKind::Replace;
    stateOps[1].value.valueKind = XGameStreamingTouchControlsStateValueKind::Boolean;
    stateOps[1].value.booleanValue = m_aEnabled;

    //B opacity
    stateOps[2].path = "/B_Opacity";
    stateOps[2].operationKind = XGameStreamingTouchControlsStateOperationKind::Replace;
    stateOps[2].value.valueKind = XGameStreamingTouchControlsStateValueKind::Double;
    stateOps[2].value.doubleValue = m_bOpacity;

    //Send the updated state
    XGameStreamingUpdateTouchControlsState(3, stateOps);
}
#pragma endregion
