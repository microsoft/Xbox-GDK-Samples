//--------------------------------------------------------------------------------------
// GameInputSequential.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameInputSequential.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace Moves
{
    const GameInputGamepadButtons Fireball[] =
    {
        GameInputGamepadDPadDown,
        GameInputGamepadDPadDown | GameInputGamepadDPadRight,
        GameInputGamepadDPadRight,
        GameInputGamepadX
    };

    const uint64_t FireballTimes[] =
    {
        0,
        500000,
        500000,
        500000
    };

    const GameInputGamepadButtons UpperCut[] =
    {
        GameInputGamepadDPadRight,
        GameInputGamepadDPadDown,
        GameInputGamepadDPadDown | GameInputGamepadDPadRight,
        GameInputGamepadX
    };

    const uint64_t UpperCutTimes[] =
    {
        0,
        500000,
        500000,
        500000
    };

    const GameInputGamepadButtons Torpedo[] =
    {
        GameInputGamepadDPadRight,
        GameInputGamepadNone,
        GameInputGamepadDPadRight,
        GameInputGamepadX
    };

    const uint64_t TorpedoTimes[] =
    {
        0,
        500000,
        500000,
        500000
    };
}

namespace
{
    void CALLBACK OnGameInputDeviceAddedRemoved(
        _In_ GameInputCallbackToken,
        _In_ void * context,
        _In_ IGameInputDevice * device,
        _In_ uint64_t,
        _In_ GameInputDeviceStatus currentStatus,
        _In_ GameInputDeviceStatus previousStatus) noexcept
    {
        auto sample = reinterpret_cast<Sample*>(context);

        bool wasConnected = (previousStatus & GameInputDeviceConnected) != 0;
        bool isConnected = (currentStatus & GameInputDeviceConnected) != 0;

        if (isConnected && !wasConnected && !sample->m_device)
        {
            sample->m_device = device;

            //We have a new device, so get the current reading as a frame of reference for future readings
            sample->m_gameInput->GetCurrentReading(GameInputKindGamepad, sample->m_device.Get(), &sample->m_lastReading);
        }
        else if(!isConnected && wasConnected && sample->m_device.Get() == device) // [SAMPLE] Device disconnected
        {
            sample->m_device.Reset();
            sample->m_lastReading.Reset();
        }
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_leftTrigger(0),
    m_rightTrigger(0),
    m_leftStickX(0),
    m_leftStickY(0),
    m_rightStickX(0),
    m_rightStickY(0),
    m_deviceToken(0),
    m_scale(1.25f)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceToken)
    {
        if (m_gameInput)
        {
#if GAMEINPUT_API_VERSION >= 1
            std::ignore = m_gameInput->UnregisterCallback(m_deviceToken);
#else
            std::ignore = m_gameInput->UnregisterCallback(m_deviceToken, UINT64_MAX);
#endif
        }

        m_deviceToken = 0;
    }

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

    m_moves.emplace_back(InputMove(L"Fireball", Moves::Fireball, Moves::FireballTimes));
    m_moves.emplace_back(InputMove(L"UpperCut", Moves::UpperCut, Moves::UpperCutTimes));
    m_moves.emplace_back(InputMove(L"Torpedo", Moves::Torpedo, Moves::TorpedoTimes));

    HRESULT hr = GameInputCreate(&m_gameInput);

#ifdef _GAMING_XBOX
    DX::ThrowIfFailed(hr);
#else
    extern LPCWSTR g_szAppName;

    if (FAILED(hr))
    {
        wchar_t buff[256] = {};
        swprintf_s(buff,
            L"GameInput creation failed with error: %08X\n\nVerify that GameInputRedist.msi has been installed as noted in the README.",
            static_cast<unsigned int>(hr));
        std::ignore = MessageBoxW(window, buff, g_szAppName, MB_ICONERROR | MB_OK);
        ExitSample();
    }
#endif

    DX::ThrowIfFailed(m_gameInput->RegisterDeviceCallback(nullptr, GameInputKindGamepad, GameInputDeviceConnected, GameInputBlockingEnumeration, this, OnGameInputDeviceAddedRemoved, &m_deviceToken));
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

    GameInputGamepadState currentState;

    if (m_lastReading)
    {

        if (m_lastReading->GetGamepadState(&currentState))
        {
            m_buttonString = L"Buttons pressed:  ";

            int exitComboPressed = 0;

            if (currentState.buttons & GameInputGamepadDPadUp)
            {
                m_buttonString += L"[DPad]Up ";
            }

            if (currentState.buttons & GameInputGamepadDPadDown)
            {
                m_buttonString += L"[DPad]Down ";
            }

            if (currentState.buttons & GameInputGamepadDPadRight)
            {
                m_buttonString += L"[DPad]Right ";
            }

            if (currentState.buttons & GameInputGamepadDPadLeft)
            {
                m_buttonString += L"[DPad]Left ";
            }

            if (currentState.buttons & GameInputGamepadA)
            {
                m_buttonString += L"[A] ";
            }

            if (currentState.buttons & GameInputGamepadB)
            {
                m_buttonString += L"[B] ";
            }

            if (currentState.buttons & GameInputGamepadX)
            {
                m_buttonString += L"[X] ";
            }

            if (currentState.buttons & GameInputGamepadY)
            {
                m_buttonString += L"[Y] ";
            }

            if (currentState.buttons & GameInputGamepadLeftShoulder)
            {
                m_buttonString += L"[LB] ";
                exitComboPressed += 1;
            }

            if (currentState.buttons & GameInputGamepadRightShoulder)
            {
                m_buttonString += L"[RB] ";
                exitComboPressed += 1;
            }

            if (currentState.buttons & GameInputGamepadLeftThumbstick)
            {
                m_buttonString += L"[LThumb] ";
            }

            if (currentState.buttons & GameInputGamepadRightThumbstick)
            {
                m_buttonString += L"[RThumb] ";
            }

            if (currentState.buttons & GameInputGamepadMenu)
            {
                m_buttonString += L"[Menu] ";
                exitComboPressed += 1;
            }

            if (currentState.buttons & GameInputGamepadView)
            {
                m_buttonString += L"[View] ";
                exitComboPressed += 1;
            }

            m_leftTrigger = currentState.leftTrigger;
            m_rightTrigger = currentState.rightTrigger;
            m_leftStickX = currentState.leftThumbstickX;
            m_leftStickY = currentState.leftThumbstickY;
            m_rightStickX = currentState.rightThumbstickX;
            m_rightStickY = currentState.rightThumbstickY;

            if (exitComboPressed == 4)
                ExitSample();
        }
    }

    while (m_lastReading)
    {
        Microsoft::WRL::ComPtr<IGameInputReading> nextReading;
        HRESULT hr = m_gameInput->GetNextReading(m_lastReading.Get(), GameInputKindGamepad, m_device.Get(), &nextReading);

        // A failure breaks us out of the loop.
        if (FAILED(hr))
        {
            // A "reading not found" error means that we've simply reached
            // the end of the input history for this device.  Other errors
            // mean something worse has happened (e.g. device disconnected),
            // so reset our state if any of those happen.
            if (hr != GAMEINPUT_E_READING_NOT_FOUND)
            {
                m_gameInput->GetCurrentReading(GameInputKindGamepad, m_device.Get(), &m_lastReading);
            }

            break;
        }

        // Otherwise, make this the new last known reading and keep going.
        m_lastReading.Swap(nextReading);
        uint64_t newTime = m_lastReading->GetTimestamp();

        if (m_lastReading->GetGamepadState(&currentState))
        {
            for (size_t i = 0; i < m_moves.size(); i++)
            {
                if (m_moves[i].Timing[m_moves[i].lastIndex] > 0 &&
                    newTime - m_moves[i].lastTime >= m_moves[i].Timing[m_moves[i].lastIndex])
                {
                    m_moves[i].Reset();
                }

                if (m_moves[i].Buttons[m_moves[i].lastIndex] == currentState.buttons)
                {
                    m_moves[i].lastIndex++;
                    m_moves[i].lastTime = newTime;

                    if (m_moves[i].lastIndex == COMBOCOUNT)
                    {
                        //Combo complete
                        m_moveString = m_moves[i].Name;
                        m_moves[i].Reset();
                    }
                }
            }
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

    wchar_t tempString[256] = {};
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

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

        swprintf(tempString, 255, L"Last completed move: %s", m_moveString.c_str());
        m_font->DrawString(m_batch.get(), tempString, pos, ATG::Colors::White);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        pos.y += m_font->GetLineSpacing() * 4.f;

        swprintf(tempString, 255, L"Supported Moves:");
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Fireball: [DPad]Down [DPad]DownRight [DPad]Right [X]");
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"UpperCut: [DPad]Right [DPad]Down [DPad]DownRight [X]");
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;

        swprintf(tempString, 255, L"Torpedo: [DPad]Right [DPad]Right [X]");
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_font->GetLineSpacing() * 1.5f;
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"No controller connected", pos, ATG::Colors::Orange);
    }

    DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), L"[RB][LB][View][Menu] Exit",
        XMFLOAT2(float(safeRect.left), float(safeRect.bottom) - m_font->GetLineSpacing()), ATG::Colors::LightGrey, m_scale);
    
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
    const auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
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

void Sample::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        const SpriteBatchPipelineStateDescription pd(
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

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
