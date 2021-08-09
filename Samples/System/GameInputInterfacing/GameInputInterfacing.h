//--------------------------------------------------------------------------------------
// GameInputInterfacing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <GameInput.h>

#include "UIManager.h"
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

using namespace ATG::UITK;

const int c_maxDevices = 4;

enum InputTypes
{
    Gamepad = 0,
    Arcade,
    Axis,
    Buttons,
    FlightStick,
    Keys,
    Mouse,
    Switches,
    Touch,
    Wheel,
    Motion,
    UINavigation,
    Count
};

struct UIDevice
{
    bool needDelete = false;
    Microsoft::WRL::ComPtr<IGameInputDevice> device;
    UIElementPtr deviceElement;
    UIElementPtr elements[InputTypes::Count];

    void Reset()
    {
        needDelete = false;
        device.Reset();
        deviceElement = nullptr;

        for (int i = 0; i < InputTypes::Count; i++)
        {
            elements[i] = nullptr;
        }
    }
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

    UIDevice    m_devices[c_maxDevices];

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    UIElementPtr GetArcadeUI(int index);
    UIElementPtr GetAxisUI(int index);
    UIElementPtr GetButtonsUI(int index);
    UIElementPtr GetFlightStickUI(int index);
    UIElementPtr GetGamepadUI(int index);
    UIElementPtr GetKeysUI(int index);
    UIElementPtr GetMotionUI(int index);
    UIElementPtr GetMouseUI(int index);
    UIElementPtr GetSwitchesUI(int index);
    UIElementPtr GetTouchUI(int index);
    UIElementPtr GetUINavigationUI(int index);
    UIElementPtr GetWheelUI(int index);
    void UpdateDeviceUI(int index);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    //Gamepad states
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    GameInputCallbackToken                      m_deviceToken;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UITK members
    ATG::UITK::UIManager          m_uiManager;

    enum Descriptors
    {
        PrintFont,
        Count,
    };
};
