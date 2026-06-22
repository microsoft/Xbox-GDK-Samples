//--------------------------------------------------------------------------------------
// CloudReactiveUI.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CloudReactiveUI.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

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
                        //For this sample, define a small screen as less than 13000 square mm
                        if (clientWidthMm * clientHeightMm < 13000)
                        {
                            sample->m_clients[i].smallScreen = true;
                        }
                    }

                    XGameStreamingIsTouchInputEnabled(sample->m_clients[i].id, &sample->m_clients[i].isTouch);

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
    m_smallScreen(false),
    m_selectedVertical(false),
    m_isTouch(false),
    m_buttonDown(false),
    m_touchDown(false),
    m_selected(0)
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

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());
    m_UIRoot = m_uiManager.GetRootElement()->GetChildByIndex(0)->GetChildByIndex(0);
    RotateScreen();

    DX::ThrowIfFailed(XTaskQueueCreate(XTaskQueueDispatchMode::Immediate, XTaskQueueDispatchMode::Immediate, &m_queue));

    DX::ThrowIfFailed(XGameStreamingInitialize());
    DX::ThrowIfFailed(XGameStreamingRegisterConnectionStateChanged(m_queue, this, ConnectionStateChangedCallback, &m_token));
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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    //Get only gamepad and touch input types
    HRESULT hr = m_gameInput->GetCurrentReading(GameInputKindGamepad | GameInputKindTouch, nullptr, &m_reading);

    if (SUCCEEDED(hr))
    {
        uint32_t inputCount;
        float touchPosX = 0.f;
        float touchPosY = 0.f;
        bool pressed = false;

        GameInputGamepadState gamepadState;

        if (m_reading->GetGamepadState(&gamepadState))
        {
            if (!m_buttonDown)
            {
                if (gamepadState.buttons & GameInputGamepadA && !m_buttonDown)
                {
                    pressed = true;
                    m_buttonDown = true;

                    if (m_selected == 4)
                    {
                        m_selectedVertical = !m_selectedVertical;

                        RotateScreen();
                    }
                    else if (m_selected == 5)
                    {
                        ExitSample();
                    }
                }
                else if (gamepadState.buttons & GameInputGamepadDPadUp && !m_buttonDown)
                {
                    m_buttonDown = true;

                    if (m_selected > 0)
                    {
                        m_selected--;
                    }
                }
                else if (gamepadState.buttons & GameInputGamepadDPadDown && !m_buttonDown)
                {
                    m_buttonDown = true;
                    if (m_selected < c_buttonCount - 1)
                    {
                        m_selected++;
                    }
                }

                UpdateMenuStyles(touchPosX, touchPosY, pressed);
            }
            else if (!(gamepadState.buttons & GameInputGamepadDPadUp)
                && !(gamepadState.buttons & GameInputGamepadDPadDown)
                && !(gamepadState.buttons & GameInputGamepadA))
            {
                m_buttonDown = false;
            }
        }

        inputCount = m_reading->GetTouchCount();
        if (!m_touchDown && inputCount > 0)
        {
            auto touchReading = std::make_unique<GameInputTouchState[]>(inputCount);
            m_reading->GetTouchState(inputCount, touchReading.get());
            touchPosX = touchReading[0].positionX;
            touchPosY = touchReading[0].positionY;

            UpdateMenuStyles(touchPosX, touchPosY, pressed);
            m_touchDown = true;
        }
        else
        {
            m_touchDown = false;
        }
    }

    m_uiManager.Update((float)timer.GetElapsedSeconds(), UIInputState());
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

    m_uiManager.Render();

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

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}
#pragma endregion

//--------------------------------------------------------------------------------------
// Name: UpdateClientState()
// Desc: Updates overlay and screen size state for clients
//--------------------------------------------------------------------------------------
void Sample::UpdateClientState()
{
    m_smallScreen = false;

    for (int i = 0; i < c_maxClients; i++)
    {
        if (m_clients[i].id != XGameStreamingNullClientId)
        {
            if (m_clients[i].smallScreen)
            {
                m_smallScreen = true;
            }
        }
    }

    m_isTouch = false;

    for (int i = 0; i < c_maxClients; i++)
    {
        if (m_clients[i].id != XGameStreamingNullClientId)
        {
            if (m_clients[i].isTouch)
            {
                m_isTouch = true;
            }
        }
    }

    //Set screen style
    RotateScreen();

    if (m_smallScreen)
    {
        //Render text larger
        for (size_t i = 0; i < c_buttonCount * 2; i++)
        {
            m_UIRoot->GetChildByIndex(i)->GetSubElementByIndex(0)->SetStyleId(ID("button_label_style_rot"));
        }
    }
    else
    {
        for (size_t i = 0; i < c_buttonCount * 2; i++)
        {
            m_UIRoot->GetChildByIndex(i)->GetSubElementByIndex(0)->SetStyleId(ID("button_label_style"));
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: UpdateMenuStyles()
// Desc: Updates overlay and screen size state for clients
//--------------------------------------------------------------------------------------
void Sample::UpdateMenuStyles(float touchPosX, float touchPosY, bool pressed)
{
    for (size_t i = 0; i < c_buttonCount; i++)
    {
        m_UIRoot->GetChildByIndex(i)->SetStyleId(ID("Basic_Button_Style"));
        m_UIRoot->GetChildByIndex(i + c_buttonCount)->SetStyleId(ID("Basic_Button_Style"));
        m_UIRoot->GetChildByIndex(i + c_buttonCount + c_buttonCount)->SetStyleId(ID("Basic_Button_Style"));
    }

    if (touchPosX != 0 && touchPosY != 0)
    {
        int localX;
        int localY;

        if (m_selectedVertical)
        {
            localY = int(touchPosX * 1920.f);
            localX = int(touchPosY * 1080.f);
        }
        else
        {
            localX = int(touchPosX * 1920.f);
            localY = int(touchPosY * 1080.f);
        }

        UIElementPtr currentButton = m_uiManager.GetVisibleElementUnderPixel(localX, localY);
        std::string id;
        bool found = false;

        do
        {
            id = currentButton->GetID().AsStr();

            if (id.size() > 6 && id.substr(0, 6) == "button")
            {
                found = true;
                break;
            }

            currentButton = currentButton->GetParent();
        } while (currentButton != nullptr);

        if (found)
        {
            //ASCII number - 48 = int value
            m_selected = (uint8_t)(id.back() - 48);

            if (id.at(id.size() - 2) == '1')
            {
                //Menu value is two characters
                m_selected += 10;
            }

            //Account for double buttons (vertical and horizontal alignment)
            m_selected %= c_buttonCount;

            currentButton->SetStyleId(ID("Pressed_Button_Style"));

            if (m_selected == 4)
            {
                m_selectedVertical = !m_selectedVertical;

                RotateScreen();
            }
            else if (m_selected == 5)
            {
                ExitSample();
            }
        }
    }
    else if (pressed)
    {
        m_UIRoot->GetChildByIndex((size_t)m_selected)->SetStyleId(ID("Pressed_Button_Style"));
        m_UIRoot->GetChildByIndex((size_t)m_selected + c_buttonCount)->SetStyleId(ID("Pressed_Button_Style"));
        m_UIRoot->GetChildByIndex((size_t)m_selected + c_buttonCount + c_buttonCount)->SetStyleId(ID("Pressed_Button_Style"));
    }
    else
    {
        m_UIRoot->GetChildByIndex((size_t)m_selected)->SetStyleId(ID("Focused_Button_Style"));
        m_UIRoot->GetChildByIndex((size_t)m_selected + c_buttonCount)->SetStyleId(ID("Focused_Button_Style"));
        m_UIRoot->GetChildByIndex((size_t)m_selected + c_buttonCount + c_buttonCount)->SetStyleId(ID("Focused_Button_Style"));
    }
}

//--------------------------------------------------------------------------------------
// Name: RotateScreen()
// Desc: Rotates screen to match current mode
//--------------------------------------------------------------------------------------
void Sample::RotateScreen()
{
    if (m_selectedVertical)
    {
        m_uiManager.SetRotation(UIRotation::Rotate270);
        m_uiManager.GetRootElement()->GetChildByIndex(0)->SetStyleId(ID("background_style_rot"));
        m_uiManager.GetRootElement()->GetChildByIndex(0)->SetRelativeSizeInRefUnits(Vector2(1080, 1920));

        m_UIRoot->SetRelativeSizeInRefUnits(Vector2(900, 1500));
        m_UIRoot->SetRelativePositionInRefUnits(Vector2(100, 100));

        for (size_t i = 0; i < c_buttonCount; i++)
        {
            m_UIRoot->GetChildByIndex(i)->SetVisible(false);
            m_UIRoot->GetChildByIndex(i + c_buttonCount)->SetVisible(false);
            m_UIRoot->GetChildByIndex(i + c_buttonCount + c_buttonCount)->SetVisible(true);
        }
    }
    else
    {
        m_uiManager.SetRotation(UIRotation::Identity);
        m_uiManager.GetRootElement()->GetChildByIndex(0)->SetStyleId(ID("background_style"));
        m_uiManager.GetRootElement()->GetChildByIndex(0)->SetRelativeSizeInRefUnits(Vector2(1920,1080));

        if (m_isTouch)
        {
            m_UIRoot->SetRelativeSizeInRefUnits(Vector2(1100, 1000));
            m_UIRoot->SetRelativePositionInRefUnits(Vector2(750, 40));
        }
        else
        {
            m_UIRoot->SetRelativeSizeInRefUnits(Vector2(700, 900));
            m_UIRoot->SetRelativePositionInRefUnits(Vector2(1100, 100));
        }

        for (size_t i = 0; i < c_buttonCount; i++)
        {
            if (m_isTouch)
            {
                m_UIRoot->GetChildByIndex(i)->SetVisible(false);
                m_UIRoot->GetChildByIndex(i + c_buttonCount)->SetVisible(true);
            }
            else
            {
                m_UIRoot->GetChildByIndex(i)->SetVisible(true);
                m_UIRoot->GetChildByIndex(i + c_buttonCount)->SetVisible(false);
            }

            m_UIRoot->GetChildByIndex(i + c_buttonCount + c_buttonCount)->SetVisible(false);
        }
    }
}
