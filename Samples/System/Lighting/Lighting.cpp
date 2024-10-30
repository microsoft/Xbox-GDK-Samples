//--------------------------------------------------------------------------------------
// Lighting.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Lighting.h"
#include "LightingEffects.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
    const wchar_t* g_EffectName[] = { L"Color Cycle", L"Color Wave", L"Color Wheel", L"Blink", L"WASD", L"Solid" };
    const wchar_t* g_EffectDesc[] =
    {
        L"Uniformly cycles colors across all lamps",
        L"Rainbow effect that moves from right to left",
        L"Rainbow effect that moves in a circle",
        L"Fades a single color off and on across all lamps",
        L"WASD keys set to a different color than the rest of the keyboard",
        L"Single random color displayed across all lamps"
    };
};

// Called any time a LampArray device is connected or disconnected
void Sample::LampArrayCallback(void* context, bool isAttached, ILampArray* lampArray)
{
    Sample* sample = reinterpret_cast<Sample*>(context);

    if (isAttached)
    {
        // Set up a new context for this device so our effects can operate on it
        std::shared_ptr<LampArrayContext> lampArrayContext = std::make_shared<LampArrayContext>();

        LampArrayColor emptyColor = {};
        LampArrayPosition lampPosition = {};
        LampArrayPosition boundingBox = {};
        uint32_t lampCount = lampArray->GetLampCount();

        lampArrayContext->lampArray.Swap(lampArray);
        lampArrayContext->lampColors.reset(new LampArrayColor[lampCount]);
        lampArrayContext->lampIndices.reset(new uint32_t[lampCount]);

        lampArray->GetBoundingBox(&boundingBox);
        float centerPointX = boundingBox.xInMeters / 2;
        float centerPointY = boundingBox.yInMeters / 2;

        // Set up caches for more efficient effect updates
        for (uint32_t i = 0; i < lampCount; i++)
        {
            lampArrayContext->lampColors.get()[i] = emptyColor;
            lampArrayContext->lampIndices.get()[i] = i;

            // Cache some lamp position values that will help optimize our effects
            ComPtr<ILampInfo> lampInfo;
            HRESULT hr = lampArray->GetLampInfo(i, &lampInfo);
            if(FAILED(hr))
            {
                sample->m_log->Format(L"GetLampInfo failed: %d, %08X\n", i, hr);
            }
            lampInfo->GetPosition(&lampPosition);
            lampArrayContext->lampXPositions[i] = lampPosition.xInMeters / boundingBox.xInMeters;
            lampArrayContext->lampWheelAngles[i] = atan2(lampPosition.yInMeters - centerPointY, lampPosition.xInMeters - centerPointX);
        }

        sample->m_lampArrays.insert(sample->m_lampArrays.end(), lampArrayContext);
        sample->m_log->Format(L"LampArray with VID/PID %04X/%04X connected with %d lamps\n",
                              lampArray->GetVendorId(), lampArray->GetProductId(), lampCount);
    }
    else
    {
        // Find the attached device's context and remove it
        for (auto iter = sample->m_lampArrays.begin(); iter != sample->m_lampArrays.end(); iter++)
        {
            if ((*iter)->lampArray.Get() == lampArray)
            {
                sample->m_lampArrays.erase(iter);
                sample->m_log->Format(L"LampArray with VID/PID %04X/%04X disconnected\n",
                                      lampArray->GetVendorId(), lampArray->GetProductId());
                break;
            }
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_currentLightingEffect(LightingEffect::ColorCycle),
    m_effectChanged(true)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_log = std::make_unique<DX::TextConsoleImage>();
}

Sample::~Sample()
{
    if (m_callbackToken)
    {
        // unregister the LampArray callback
        UnregisterLampArrayCallback(m_callbackToken, 5000);
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_log->WriteLine(L"Please connect a supported LampArray device...");

    // Setup the LampArray connect/disconnect callback
    HRESULT hr = RegisterLampArrayCallback(Sample::LampArrayCallback, this, &m_callbackToken);
    if(FAILED(hr))
    {
        m_log->Format(L"RegisterLampArrayCallback failed: %08X", hr);
    }
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
void Sample::Update(DX::StepTimer const& timer)
{
    UNREFERENCED_PARAMETER(timer);

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    if(m_lampArrays.size() > 0)
    {
        // Run the effect per LampArray device
        for(auto &lampArray : m_lampArrays)
        {
            // If we changed effects, reset the lamps to a known good state
            if(m_effectChanged)
            {
                LightingEffects::ResetEffects(lampArray.get());
            }

            // Run the actual effect
            // The implementations can be found in LightingEffects.cpp/.h
            switch(m_currentLightingEffect)
            {
                // These effects are dynamic and should be updated every frame
                case LightingEffect::ColorCycle:
                    LightingEffects::UpdateColorCycleEffect(lampArray.get());
                    break;
                case LightingEffect::Blink:
                    LightingEffects::UpdateBlinkEffect(lampArray.get());
                    break;
                case LightingEffect::ColorWave:
                    LightingEffects::UpdateColorWaveEffect(lampArray.get());
                    break;
                case LightingEffect::ColorWheel:
                    LightingEffects::UpdateColorWheelEffect(lampArray.get());
                    break;

                // These effects are not dynamic, so only need to be called once
                case LightingEffect::WASD:
                    if(m_effectChanged)
                    {
                        LightingEffects::UpdateWASDEffect(lampArray.get());
                    }
                    break;
                case LightingEffect::Solid:
                    if(m_effectChanged)
                    {
                        LightingEffects::UpdateSolidEffect(lampArray.get());
                    }
                    break;
            }

            lampArray->frameCount++;
        }

        m_effectChanged = false;
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
        else if(m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            PreviousEffect();
        }
        else if(m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            NextEffect();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }
    else if(m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Up))
    {
        PreviousEffect();
    }
    else if(m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Down))
    {
        NextEffect();
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

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    m_spriteBatch->Begin(commandList);
        if(m_lampArrays.size() == 0)
        {
            m_font->DrawString(m_spriteBatch.get(), L"Please connect a supported LampArray device...", XMFLOAT2(100, 100));
        }
        else
        {
            m_font->DrawString(m_spriteBatch.get(), L"Select an effect using the arrow keys or the DPad.", XMFLOAT2(100, 100));
            m_font->DrawString(m_spriteBatch.get(), L"Effect Description:", XMFLOAT2(640, 196));
            DX::DrawControllerString(m_spriteBatch.get(), m_font.get(), m_ctrlFont.get(), L"[Dpad] / Arrows - Change Effect\r\n[View] / Esc - Exit", XMFLOAT2(80, 960));

            for (uint32_t i = 0; i < NUM_EFFECTS; i++)
            {
                bool current = (i == static_cast<uint32_t>(m_currentLightingEffect));
                m_font->DrawString(m_spriteBatch.get(), g_EffectName[i], XMFLOAT2(140, 200 + (i * 32.0f)), current ? Colors::Yellow : Colors::White);
                if(current)
                {
                    m_font->DrawString(m_spriteBatch.get(), L"->", XMFLOAT2(100, 200 + (i * 32.0f)), Colors::Yellow);
                    m_font->DrawString(m_spriteBatch.get(), g_EffectDesc[i], XMFLOAT2(640, 230), Colors::Yellow);
                }
            }
        }
    m_spriteBatch->End();

    m_log->Render(commandList);

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
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
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
    width = 1920;
    height = 1080;
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
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    wchar_t font[MAX_PATH];
    DX::FindMediaFile(font, MAX_PATH, L"courier_16.spritefont");

    wchar_t background[MAX_PATH];
    DX::FindMediaFile(background, MAX_PATH, L"ATGSampleBackground.DDS");

    m_log->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
    );

    DX::FindMediaFile(font, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        font,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(font, MAX_PATH, L"SegoeUI_24.spritefont");
    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        font,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const viewport = m_deviceResources->GetScreenViewport();

    static const RECT logSize = { 980, 800, 1780, 1050 };
    m_log->SetWindow(logSize, false);
    m_log->SetViewport(viewport);

    m_spriteBatch->SetViewport(viewport);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_resourceDescriptors.reset();

}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
