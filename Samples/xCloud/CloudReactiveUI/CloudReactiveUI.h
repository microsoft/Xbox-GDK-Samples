//--------------------------------------------------------------------------------------
// CloudReactiveUI.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"

#include "xgamestreaming.h"

const uint8_t c_maxClients = 4;
const uint8_t c_buttonCount = 6;

struct ClientDevice
{
    XGameStreamingClientId id;
    bool smallScreen;
    bool isTouch;

    ClientDevice()
    {
        id = XGameStreamingNullClientId;
        smallScreen = false;
        isTouch = false;
    }
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

    void UpdateClientState();

    ClientDevice m_clients[c_maxClients];

private:

    void UpdateMenuStyles(float touchPosX, float touchPosY, bool pressed);
    void RotateScreen();

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;

    //Input
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    bool                                        m_smallScreen;
    bool                                        m_selectedVertical;
    bool                                        m_isTouch;
    bool                                        m_buttonDown;
    bool                                        m_touchDown;
    uint8_t                                     m_selected;

    XTaskQueueRegistrationToken                 m_token = {};
    XTaskQueueHandle                            m_queue = nullptr;
    ATG::UITK::UIElementPtr                     m_UIRoot;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
};
