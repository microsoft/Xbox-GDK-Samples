//--------------------------------------------------------------------------------------
// GamepadKeyboardMouse.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GamepadKeyboardMouse.h"
#include <iomanip>
#include <sstream>

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_cursorState((maxX-minX)/2 + minX, (maxY - minY) / 2 + minY),
    m_cursorInput(),
    m_lastCursorInput(),
    m_leftTrigger(0),
    m_rightTrigger(0),
    m_leftStickX(0),
    m_leftStickY(0),
    m_rightStickX(0),
    m_rightStickY(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_logConsole = std::make_unique<DX::TextConsoleImage>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    m_gameInput->UnregisterCallback(deviceCallbackToken, UINT64_MAX);

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
    if(!m_cursorState.cursorVisible)
        ShowCursor(true);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // [GameInput]
    // Initializes GameInput and is called once for the lifetime of the application
    HRESULT hr = GameInputCreate(&m_gameInput);
    DX::ThrowIfFailed(hr);

    // [GameInput]
    // Tells GameInput to listen for device events. Several filters can be used for different use cases
    hr = m_gameInput->RegisterDeviceCallback(
        nullptr,
        GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse,
        GameInputDeviceAnyStatus,
        GameInputAsyncEnumeration,
        this,
        &DeviceCallback,
        &deviceCallbackToken);

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
            Update();
        });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    HRESULT hr;
    ComPtr<IGameInputDevice> device;

    // Track the input for the Cursor Logic
    m_cursorInput = CursorInput{};

    // Gamepad Input
    {
        // [GameInput]
        // Check for a reading from a device. GameInputKind can be used to filter by certain device types
        hr = m_gameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &m_reading);
        // Success indicates gamepad is connected
        if (SUCCEEDED(hr))
        {
            GameInputGamepadState state = {};

            // [GameInput]
            // Reading objects contain the input states of their devices
            if (m_reading->GetGamepadState(&state))
            {
                static int rotationMultiplier = 10;
                m_cursorInput.CursorInputAxisX = state.leftThumbstickX;
                m_cursorInput.CursorInputAxisY = state.leftThumbstickY;
                m_cursorInput.CursorInputRotation = static_cast<int>(state.rightThumbstickX * rotationMultiplier);

                m_leftTrigger = state.leftTrigger;
                m_rightTrigger = state.rightTrigger;
                m_leftStickX = state.leftThumbstickX;
                m_leftStickY = state.leftThumbstickY;
                m_rightStickX = state.rightThumbstickX;
                m_rightStickY = state.rightThumbstickY;

                m_buttonString = L"Buttons pressed:  ";

                int exitComboPressed = 0;

                if (state.buttons & GameInputGamepadDPadUp)
                {
                    m_buttonString += L"[DPad]Up ";
                    m_cursorInput.CursorInputAxisY = 1;
                }

                if (state.buttons & GameInputGamepadDPadDown)
                {
                    m_buttonString += L"[DPad]Down ";
                    m_cursorInput.CursorInputAxisY = -1;
                }

                if (state.buttons & GameInputGamepadDPadRight)
                {
                    m_buttonString += L"[DPad]Right ";
                    m_cursorInput.CursorInputAxisX = 1;
                }

                if (state.buttons & GameInputGamepadDPadLeft)
                {
                    m_buttonString += L"[DPad]Left ";
                    m_cursorInput.CursorInputAxisX = -1;
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

                if (exitComboPressed == 4)
                    ExitSample();
            }
        }
    }

    // Keyboard Input
    {
        m_keyboardString.clear();

        // [GameInput]
        // Check for a reading from a device. GameInputKind can be used to filter by certain device types
        hr = m_gameInput->GetCurrentReading(GameInputKindKeyboard, nullptr, &m_reading);
        // Success indicates keyboard is connected
        if (SUCCEEDED(hr))
        {
            const uint32_t count = m_reading->GetKeyCount();
            if (count > 0)
            {
                // Keyboards rarely support more than 12-16 keys at once
                GameInputKeyState keyboardState[16];

                // [GameInput]
                // Reading objects contain the input states of their devices
                if (m_reading->GetKeyState(_countof(keyboardState), keyboardState))
                {
                    int exitComboPressed = 0;

                    for (uint32_t index = 0; index < count; index++)
                    {
                        // [GameInput]
                        // Virtual keys are a good way to track which key was pressed
                        // Scan codes are also good for locating the physical location of keys on keyboards
                        auto vKey = keyboardState[index].virtualKey;
                        // Example of using virutal key value for 'D' and virtual key constant for 'right arrow'
                        if (vKey == 'D' || vKey == VK_RIGHT) {
                            m_cursorInput.CursorInputAxisX = 1.f;
                        }
                        else if (vKey == 'A' || vKey == VK_LEFT) {
                            m_cursorInput.CursorInputAxisX = -1.f;
                        }
                        if (vKey == 'W' || vKey == VK_UP) {
                            m_cursorInput.CursorInputAxisY = 1.f;
                        }
                        else if (vKey == 'S' || vKey == VK_DOWN) {
                            m_cursorInput.CursorInputAxisY = -1.f;
                        }
                        if (vKey == VK_LCONTROL || vKey == VK_LSHIFT || vKey == 'X') {
                            exitComboPressed += 1;
                        }

                        // Change mouse mode
                        m_cursorInput.CursorInputSwitch = vKey == '\t';

                        wchar_t buff[128] = {};
                        swprintf_s(buff, L"Scan Code: 0x%02X    VKey: 0x%02X, %ls\n",
                            keyboardState[index].scanCode,
                            keyboardState[index].virtualKey,
                            c_vkeyNames[keyboardState[index].virtualKey]);
                        m_keyboardString += buff;
                    }

                    if (exitComboPressed == 3)
                        ExitSample();
                }
            }
            else {
                m_keyboardString += L"No keys pressed";
            }
        }
    }

    // Mouse GameInput Input
    // Managing mouse inputs and relative mouse position, based on deltas

    {
        m_mouseGiString.clear();

        // [GameInput]
        // At time of writing (Feb 2023), GameInput does not support touchpad mouses
        hr = m_gameInput->GetCurrentReading(GameInputKindMouse, nullptr, &m_reading);
        // Success indicates mouse is connected
        if (SUCCEEDED(hr))
        {
            static int64_t lastDeltaX = 0;
            static int64_t lastDeltaY = 0;
            static int64_t lastDeltaWheel = 0;
            static GameInputMouseState lastState;

            // [GameInput]
            // Reading objects contain the input states of their devices
            GameInputMouseState state;
            if (m_reading->GetMouseState(&state))
            {
                int64_t deltaX = state.positionX - lastState.positionX;
                int64_t deltaY = state.positionY - lastState.positionY;
                int64_t deltaWheel = state.wheelY - lastState.wheelY;

                lastDeltaX = deltaX ? deltaX : lastDeltaX;
                lastDeltaY = deltaY ? deltaY : lastDeltaY;
                lastDeltaWheel = deltaWheel ? deltaWheel : static_cast<int>(lastDeltaWheel);

                lastState = state;

                // Change mouse mode
                if (!m_cursorInput.CursorInputSwitch)
                    m_cursorInput.CursorInputSwitch = state.buttons & GameInputMouseRightButton;

                // Update cursor's rotation with relative change
                if (m_cursorInput.CursorInputRotation == 0)
                    m_cursorInput.CursorInputRotation = static_cast<int>(deltaX);

                m_mouseGiString += (state.buttons == GameInputMouseNone) ? L"No buttons pressed" :
                    (state.buttons & GameInputMouseLeftButton) ? L"[Left]" : L"";
                m_mouseGiString += (state.buttons & GameInputMouseMiddleButton) ? L"[Middle]" : L"";
                m_mouseGiString += (state.buttons & GameInputMouseRightButton) ? L"[Right]" : L"";
                m_mouseGiString += (state.buttons & GameInputMouseButton4) ? L"[Button 4]" : L"";
                m_mouseGiString += (state.buttons & GameInputMouseButton5) ? L"[Button 5]" : L"";

                m_mouseGiString += L"\nAccumulated X pos: " + std::to_wstring(state.positionX) + L" + " + std::to_wstring(lastDeltaX);
                m_mouseGiString += L"\nAccumulated Y pos: " + std::to_wstring(state.positionY) + L" + " + std::to_wstring(lastDeltaY);
                m_mouseGiString += L"\nWheel: " + std::to_wstring(state.wheelY) + L" + " + std::to_wstring(lastDeltaWheel);
            }
        }
    }

    // Mouse Messaging Input
    // Managing absolute mouse position in application. GameInput only manages relative mouse position

    // [GameInput]
    // At time of writing (Feb 2023), GameInput does not support absolute mode and requires an alternative
    // solution, like Windows Messaging.
    {
        // Messaging implemented in Main.cpp (check WM_MOUSEMOVE)
        m_mouseMsgString.clear();

        m_cursorInput.CursorInputMouseX = m_screenLocation_x;
        m_cursorInput.CursorInputMouseY = m_screenLocation_y;
        m_mouseMsgString += L"\nX pos: " + std::to_wstring(m_screenLocation_x);
        m_mouseMsgString += L"\nY pos: " + std::to_wstring(m_screenLocation_y);
    }

    ProcessInput();

    m_lastCursorInput = m_cursorInput;

    PIXEndEvent();
}
#pragma endregion

void CALLBACK Sample::DeviceCallback(
    _In_ GameInputCallbackToken /*callbackToken*/,
    _In_ void* context,
    _In_ IGameInputDevice* device,
    _In_ uint64_t /*timestamp*/,
    _In_ GameInputDeviceStatus currentStatus,
    _In_ GameInputDeviceStatus previousStatus) noexcept
{
    // [GameInput]
    // GameInputDeviceInfo contains general and type-specific information of a given device
    const GameInputDeviceInfo* info = device->GetDeviceInfo();
    GameInputKind deviceKind = info->supportedInput;
    const wchar_t* deviceKindString;
    const wchar_t* deviceEventString = nullptr;

    if (deviceKind & GameInputKindGamepad)
        deviceKindString = L"Gamepad";
    else if (deviceKind & GameInputKindMouse)
        deviceKindString = L"Mouse";
    else
        deviceKindString = L"Keyboard";

    Sample* pThis = static_cast<Sample*>(context);

    if (!(previousStatus & GameInputDeviceConnected) &&
        (currentStatus & GameInputDeviceConnected))
    {
        deviceEventString = L"Connected";
    }
    else if ((previousStatus & GameInputDeviceConnected) &&
        !(currentStatus & GameInputDeviceConnected))
    {
        deviceEventString = L"Disconnected";
    }
    if (deviceEventString)
    {
        pThis->m_logConsole->Format(ATG::Colors::OffWhite, L"    %ls %ls\n", deviceKindString, deviceEventString);
    }
}

void Sample::ProcessInput()
{
    static float deadZone = .35f;

    // Check mouse mode
    if (m_cursorInput.CursorInputSwitch && !m_lastCursorInput.CursorInputSwitch)
        m_cursorState.isRelativeMode = !m_cursorState.isRelativeMode;

    // Update Position
    int xPos;
    int yPos;
    if (m_cursorState.isRelativeMode)
    {
        float xDelta = abs(m_cursorInput.CursorInputAxisX) > deadZone ? m_cursorInput.CursorInputAxisX * moveSpeed : 0;
        float yDelta = abs(m_cursorInput.CursorInputAxisY) > deadZone ? m_cursorInput.CursorInputAxisY * moveSpeed : 0;
        xPos = static_cast<int>(xDelta + m_cursorState.xPos);
        yPos = static_cast<int>(-yDelta + m_cursorState.yPos);
    }
    else
    {
        xPos = m_cursorInput.CursorInputMouseX;
        yPos = m_cursorInput.CursorInputMouseY;
    }
    m_cursorState.xPos = std::max(
        minX, std::min(xPos, maxX)
    );
    m_cursorState.yPos = std::max(
        minY, std::min(yPos, maxY)
    );

    // Update Rotation
    // Relative mode will rotate the cursor
    if (m_cursorState.isRelativeMode)
        m_cursorState.degreeRotation = (m_cursorState.degreeRotation + m_cursorInput.CursorInputRotation) % 360;

    // Mouse Display (PC)
    if (m_cursorState.cursorVisible &&
        !(m_cursorState.isRelativeMode ||
            (minX > xPos || xPos > maxX || minY > yPos || yPos > maxY)))
    {
        m_cursorState.cursorVisible = false;
        ShowCursor(false);
    }
    else if (!m_cursorState.cursorVisible &&
        (m_cursorState.isRelativeMode ||
            (minX > xPos || xPos > maxX || minY > yPos || yPos > maxY)))
    {
        m_cursorState.cursorVisible = true;
        ShowCursor(true);
    }
}

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

    auto const safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);

    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), fullscreen);

    // Cursor
    if (m_cursorState.isRelativeMode)
    {
        m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cursor), XMUINT2(cursorSize, cursorSize),
            DirectX::SimpleMath::Vector2{ (float)m_cursorState.xPos, (float)m_cursorState.yPos },
            NULL, ATG::Colors::White, DegreesToRad(m_cursorState.degreeRotation), DirectX::XMFLOAT2{ cursorSize / 2,cursorSize / 2 });
    }
    else
    {
        m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Pointer), XMUINT2(cursorSize, cursorSize),
            DirectX::SimpleMath::Vector2{ (float)m_cursorState.xPos, (float)m_cursorState.yPos });
    }

    // Input Display
    wchar_t tempString[256] = {};
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));
    float m_fontSpacing = m_font->GetLineSpacing();
    float m_smallFontSpacing = m_smallFont->GetLineSpacing();

    // ATG Logo
    static uint32_t logoW = 100;
    static uint32_t logoH = 109;
    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Logo),
        XMUINT2(logoW, logoH), pos);

    m_font->DrawString(m_batch.get(), "GameInput Gamepad Keyboard Mouse Sample", XMFLOAT2(pos.x + logoW, pos.y + logoH/4));
    pos.y += logoH * 1;

    // Mouse Mode
    m_font->DrawString(m_batch.get(), m_cursorState.isRelativeMode ? "Mouse: Relative Mode" : "Mouse: Absolute Mode",
        XMFLOAT2(static_cast<float>(minX), static_cast<float>(minY - m_fontSpacing*2)), ATG::Colors::White);

    // Gamepad
    m_font->DrawString(m_batch.get(), "Gamepad", pos, ATG::Colors::OffWhite);
    pos.y += m_fontSpacing;

    if (!m_buttonString.empty())
    {
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), m_buttonString.c_str(), pos);
        pos.y += m_smallFontSpacing;

        swprintf_s(tempString, L"[LT]  %1.3f", m_leftTrigger);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_smallFontSpacing;

        swprintf_s(tempString, L"[RT]  %1.3f", m_rightTrigger);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_smallFontSpacing;

        swprintf_s(tempString, L"[LThumb]  X: %1.3f  Y: %1.3f", m_leftStickX, m_leftStickY);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), tempString, pos);
        pos.y += m_smallFontSpacing;

        swprintf_s(tempString, L"[RThumb]  X: %1.3f  Y: %1.3f", m_rightStickX, m_rightStickY);
        DX::DrawControllerString(m_batch.get(), m_smallFont.get(), m_ctrlFont.get(), tempString, pos);
    }
    else
    {
        m_smallFont->DrawString(m_batch.get(), L"No controller connected", pos, ATG::Colors::Orange);
    }

    pos.y += m_smallFontSpacing * 1.5f;

    // GI Mouse
    m_font->DrawString(m_batch.get(), "Mouse (GameInput)", pos, ATG::Colors::OffWhite);
    pos.y += m_fontSpacing;

    if (!m_mouseGiString.empty())
    {
        m_smallFont->DrawString(m_batch.get(), m_mouseGiString.c_str(), pos);
        pos.y += m_smallFontSpacing * 4.f;
    }
    else
    {
        m_smallFont->DrawString(m_batch.get(), L"No mouse connected", pos, ATG::Colors::Orange);
        pos.y += m_smallFontSpacing * 1.5f;
    }

    // Msg Mouse
    m_font->DrawString(m_batch.get(), "Mouse (Windows Messaging)", pos, ATG::Colors::OffWhite);

    if (!m_mouseMsgString.empty())
    {
        m_smallFont->DrawString(m_batch.get(), m_mouseMsgString.c_str(), pos);
        pos.y += m_smallFontSpacing * 3.5f;
    }
    else
    {
        pos.y += m_fontSpacing;
        m_smallFont->DrawString(m_batch.get(), L"No mouse connected", pos, ATG::Colors::Orange);
        pos.y += m_smallFontSpacing * 1.5f;
    }

    // Keyboard
    m_font->DrawString(m_batch.get(), "Keyboard", pos, ATG::Colors::OffWhite);
    pos.y += m_fontSpacing;

    if (!m_keyboardString.empty())
    {
        m_smallFont->DrawString(m_batch.get(), m_keyboardString.c_str(), pos);
    }
    else
    {
        m_smallFont->DrawString(m_batch.get(), L"No keyboard connected", pos, ATG::Colors::Orange);
    }

    pos.y += m_smallFontSpacing * 1.5f;

    // Controls
    pos = XMFLOAT2(float(safeRect.left), float(safeRect.bottom) - m_smallFontSpacing);

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"[RB]+[LB]+[View]+[Menu] or LCtrl + LSfhit + X: Exit",
        pos,
        ATG::Colors::LightGrey);

    pos.y -= m_smallFontSpacing;

    DX::DrawControllerString(m_batch.get(),
        m_smallFont.get(), m_ctrlFont.get(),
        L"Tab or Right Click: Change Mouse Mode",
        pos,
        ATG::Colors::LightGrey);

    pos.y -= m_smallFontSpacing;

    if (m_cursorState.isRelativeMode)
    {
        DX::DrawControllerString(m_batch.get(),
            m_smallFont.get(), m_ctrlFont.get(),
            L"[RThumb] or Move Mouse Left/Right: Rotate Cursor",
            pos,
            ATG::Colors::LightGrey);

        pos.y -= m_smallFontSpacing;

        DX::DrawControllerString(m_batch.get(),
            m_smallFont.get(), m_ctrlFont.get(),
            L"[DPad] or WASD: Move Cursor",
            pos,
            ATG::Colors::LightGrey);
    }
    else
    {
        DX::DrawControllerString(m_batch.get(),
            m_smallFont.get(), m_ctrlFont.get(),
            L"Move Mouse: Move Cursor",
            pos,
            ATG::Colors::LightGrey);
    }

    m_batch->End();

    m_logConsole->Render(commandList);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        const SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Assets/GamerPic_Transparent.png");
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload, strFilePath,
            m_cursor.ReleaseAndGetAddressOf(), false)
    );

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Assets/GamerPic_Pointer.png");
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload, strFilePath,
            m_pointer.ReleaseAndGetAddressOf(), false)
    );

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Assets/LOGO_ATG_SMALL.png");
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload, strFilePath,
            m_logo.ReleaseAndGetAddressOf(), false)
    );

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.DDS");
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, strFilePath,
        m_background.ReleaseAndGetAddressOf()));

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

    wchar_t bgFilePath[MAX_PATH] = {};
    DX::FindMediaFile(bgFilePath, MAX_PATH, L"Assets/TransparentBox.png");

    m_logConsole->RestoreDevice(
        device,
        upload,
        rtState,
        strFilePath,
        bgFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleBackground),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleBackground)
    );

    m_logConsole->SetDebugOutput(true);

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_cursor.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Cursor));

    CreateShaderResourceView(device, m_pointer.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Pointer));

    CreateShaderResourceView(device, m_logo.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Logo));

    CreateShaderResourceView(device, m_background.Get(),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);

    static const RECT LOG_RECT = { minX - cursorSize / 2, minY - cursorSize / 2, maxX + cursorSize / 2, maxY + cursorSize / 2 };
    m_logConsole->SetWindow(LOG_RECT, false);
    m_logConsole->SetViewport(vp);
}

void Sample::OnDeviceLost()
{
     m_logConsole->ReleaseDevice();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion
