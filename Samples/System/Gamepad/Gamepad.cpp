//--------------------------------------------------------------------------------------
// Gamepad.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_deviceString{},
    m_leftTrigger(0),
    m_rightTrigger(0),
    m_leftStickX(0),
    m_leftStickY(0),
    m_rightStickX(0),
    m_rightStickY(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    HRESULT hr = GameInputCreate(&m_gameInput);
    DX::ThrowIfFailed(hr);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if (FAILED(m_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &m_reading)))
    {
        // Failure indicates no gamepad is connected
        m_buttonString.clear();
    }
    else
    {
        ComPtr<IGameInputDevice> device;
        m_reading->GetDevice(&device);

        int currentDevice = -1;

        for (size_t i = 0; i< m_devices.size(); i++)
        {
            // GameInput device objects are singletons, and their lifetime is the same as the process.
            // The API guarantees that IGameInputDevice instances can be directly compared for equality.
            if (m_devices[i].Get() == device.Get())
            {
                currentDevice = static_cast<int>(i);
                break;
            }
        }
        
        if (currentDevice == -1)
        {
            currentDevice = static_cast<int>(m_devices.size());
            m_devices.emplace_back(device);
        }

        swprintf(m_deviceString, 19, L"Gamepad index: %d", currentDevice);

        GameInputGamepadState state = {};

        if (m_reading->GetGamepadState(&state))
        {
            m_buttonString = L"Buttons pressed:  ";

            int exitComboPressed = 0;

            if (state.buttons & GameInputGamepadDPadUp)
            {
                m_buttonString += L"[DPad]Up ";
            }

            if (state.buttons & GameInputGamepadDPadDown)
            {
                m_buttonString += L"[DPad]Down ";
            }

            if (state.buttons & GameInputGamepadDPadRight)
            {
                m_buttonString += L"[DPad]Right ";
            }

            if (state.buttons & GameInputGamepadDPadLeft)
            {
                m_buttonString += L"[DPad]Left ";
            }

            if (state.buttons & GameInputGamepadA)
            {
                m_buttonString += L"[A] ";
            }

            if (state.buttons & GameInputGamepadB)
            {
                m_buttonString += L"[B] ";
            }

            if (state.buttons & GameInputGamepadX)
            {
                m_buttonString += L"[X] ";
            }

            if (state.buttons & GameInputGamepadY)
            {
                m_buttonString += L"[Y] ";
            }

            if (state.buttons & GameInputGamepadLeftShoulder)
            {
                m_buttonString += L"[LB] ";
                exitComboPressed += 1;
            }

            if (state.buttons & GameInputGamepadRightShoulder)
            {
                m_buttonString += L"[RB] ";
                exitComboPressed += 1;
            }

            if (state.buttons & GameInputGamepadLeftThumbstick)
            {
                m_buttonString += L"[LThumb] ";
            }

            if (state.buttons & GameInputGamepadRightThumbstick)
            {
                m_buttonString += L"[RThumb] ";
            }

            if (state.buttons & GameInputGamepadMenu)
            {
                m_buttonString += L"[Menu] ";
                exitComboPressed += 1;
            }

            if (state.buttons & GameInputGamepadView)
            {
                m_buttonString += L"[View] ";
                exitComboPressed += 1;
            }

            m_leftTrigger = state.leftTrigger;
            m_rightTrigger = state.rightTrigger;
            m_leftStickX = state.leftThumbstickX;
            m_leftStickY = state.leftThumbstickY;
            m_rightStickX = state.rightThumbstickX;
            m_rightStickY = state.rightThumbstickY;

            if (exitComboPressed == 4)
                ExitSample();
        }
    }

    PIXEndEvent();
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

    auto const fullscreen = m_deviceResources->GetOutputSize();

    auto const safeRect = Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    wchar_t tempString[256] = {};
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_font->DrawString(m_batch.get(), m_deviceString, pos, ATG::Colors::OffWhite);
    pos.y += m_font->GetLineSpacing() * 1.5f;
    
    if (!m_buttonString.empty())
    {
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), m_buttonString.c_str(), pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"[LT]  %1.3f", m_leftTrigger);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"[RT]  %1.3f", m_rightTrigger);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"[LThumb]  X: %1.3f  Y: %1.3f", m_leftStickX, m_leftStickY);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"[RThumb]  X: %1.3f  Y: %1.3f", m_rightStickX, m_rightStickY);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);

        pos.y += m_font->GetLineSpacing() * 2.f;
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"No controller connected", pos, ATG::Colors::Orange);
    }

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[RB]+[LB]+[View]+[Menu] Exit",
        XMFLOAT2(float(safeRect.left), float(safeRect.bottom) - m_font->GetLineSpacing()),
        ATG::Colors::LightGrey);
    
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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        const SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
    m_font = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_smallFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"gamepad.dds");
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, strFilePath,
        m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}

void Sample::OnDeviceLost()
{
    m_font.reset();
    m_smallFont.reset();
    m_ctrlFont.reset();
    m_batch.reset();
    m_resourceDescriptors.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
