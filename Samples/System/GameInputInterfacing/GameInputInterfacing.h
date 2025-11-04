//--------------------------------------------------------------------------------------
// GameInputInterfacing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

using namespace ATG::UITK;

constexpr size_t c_maxDevices = 4;

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
class Sample final : public DX::IDeviceNotify, public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
#ifdef _GAMING_XBOX
    void OnConstrained() {}
    void OnUnConstrained() {}
#endif
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
#ifdef _GAMING_XBOX
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }
#endif

    // ATG::UITK::D3DResourcesProvider
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

    UIElementPtr GetArcadeUI(size_t index);
    UIElementPtr GetAxisUI(size_t index);
    UIElementPtr GetButtonsUI(size_t index);
    UIElementPtr GetFlightStickUI(size_t index);
    UIElementPtr GetGamepadUI(size_t index);
    UIElementPtr GetKeysUI(size_t index);
    UIElementPtr GetMouseUI(size_t index);
    UIElementPtr GetSwitchesUI(size_t index);
    UIElementPtr GetTouchUI(size_t index);
    UIElementPtr GetUINavigationUI(size_t index);
    UIElementPtr GetWheelUI(size_t index);
    void UpdateDeviceUI(size_t index);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;

    //Gamepad states
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    GameInputCallbackToken                      m_deviceToken;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
};
