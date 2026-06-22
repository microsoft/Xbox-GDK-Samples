//--------------------------------------------------------------------------------------
// RawDevice.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RawDevice.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace Xbox;

namespace
{
    void CALLBACK OnGameInputDeviceAddedRemoved(
        _In_ GameInputCallbackToken,
        _In_ void * context,
        _In_ IGameInputDevice * device,
        _In_ uint64_t,
        _In_ GameInputDeviceStatus currentStatus,
        _In_ GameInputDeviceStatus) noexcept
    {
        auto sample = reinterpret_cast<Sample*>(context);

        sample->m_deviceLock.lock();

        if (currentStatus & GameInputDeviceConnected)
        {
            RawWheel newWheel;

            if (newWheel.Init(sample->m_gameInput.Get(), device))
            {
                // Only add the new wheel if the underlying device isn't already in our list
                auto element = std::find(sample->m_devices.begin(), sample->m_devices.end(), newWheel);
                if (element == sample->m_devices.end())
                {
                    sample->m_devices.emplace_back(newWheel);
                }
            }
        }
        else
        {
            for (auto it = sample->m_devices.begin(); it != sample->m_devices.end(); ++it)
            {
                if (it->CompareDevice(device))
                {
                    sample->m_devices.erase(it);
                    break;
                }
            }
        }

        sample->m_deviceLock.unlock();
    }
}

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_scale(1.25f)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    //Only watch for racing wheels that support raw device reports
    DX::ThrowIfFailed(m_gameInput->RegisterDeviceCallback(nullptr,
        GameInputKindRacingWheel,
        GameInputDeviceConnected,
        GameInputBlockingEnumeration,
        this,
        OnGameInputDeviceAddedRemoved,
        &m_deviceToken));
}

Sample::~Sample()
{
    if (m_deviceToken)
    {
        if (m_gameInput)
        {
            std::ignore = m_gameInput->UnregisterCallback(m_deviceToken, UINT64_MAX);
        }

        m_deviceToken = 0;
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

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

    m_deviceLock.lock();

    for (auto& it : m_devices)
    {
        it.Update();
    }

    m_deviceLock.unlock();
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

    wchar_t tempString[256] = {};
    int i = 1;
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_deviceLock.lock();

    for (auto& it : m_devices)
    {
        swprintf(tempString, 255, L"RawWheel #%d", i);
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), it.DeviceString(), pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), it.ButtonString(), pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        swprintf(tempString, 255, L"Wheel  %1.3f    Clutch  %1.3f", it.Wheel(), it.Clutch());
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        swprintf(tempString, 255, L"Brake  %1.3f    Throttle  %1.3f", it.Brake(), it.Throttle());
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        swprintf(tempString, 255, L"Handbrake  %1.3f    Shifter  %d", it.Handbrake(), it.Gear());
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * m_scale;

        m_font->DrawString(m_batch.get(), L"Reading: ", pos, ATG::Colors::OffWhite);
        pos.y += m_font->GetLineSpacing() * m_scale;

        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), it.RawMessageString(), pos);

        pos.y += m_font->GetLineSpacing() * m_scale * 2;
        i++;
    }

    m_deviceLock.unlock();

    if(m_devices.empty())
    {
        m_font->DrawString(m_batch.get(), L"No compatible device connected", pos, ATG::Colors::Orange);
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

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_24.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::PrintFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::PrintFont));

    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        L"XboxOneControllerLegendSmall.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.dds", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}
#pragma endregion
