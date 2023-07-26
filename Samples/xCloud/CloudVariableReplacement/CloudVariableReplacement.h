//--------------------------------------------------------------------------------------
// CloudVariableReplacement.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "xgamestreaming.h"
#include <unordered_map>
#include <string>

const int c_maxClients = 4;

struct ClientDevice
{
    XGameStreamingClientId id;
    XTaskQueueRegistrationToken propertiesChangedRegistration;
    bool validOverlay;
    bool smallScreen;

    ClientDevice()
    {
        id = XGameStreamingNullClientId;
        propertiesChangedRegistration = {};
        validOverlay = false;
        smallScreen = false;
    }
};

// Available touch layouts in the included "sample-layouts" bundle
enum class TouchLayout
{
    Standard = 0,
    Fighting = 1 
};

const std::unordered_map<TouchLayout, std::string> c_touchLayoutNames{
    {TouchLayout::Standard, "standard-variable-replacement"},
    {TouchLayout::Fighting, "fighting-variable-replacement"}
};

// The constraints on what the minimum (inclusive) and maximum (exclusive)
// bundle versions are supported.
const XVersion c_minimumBundleversion{0, 0, 0, 1};
const XVersion c_maximumBundleVersion{1, 0, 0, 0};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
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
    XTaskQueueHandle GetTaskQueue() const noexcept { return m_queue; };

    ClientDevice* GetClientById(XGameStreamingClientId clientId);
    void UpdateClientState();

    void UpdateOverlayState(XGameStreamingClientId clientId);

    ClientDevice m_clients[c_maxClients];

private:
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

    // Input device.
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    std::wstring			                    m_typeString;

    // Game state.
    bool										m_buttonDown;
    bool                                        m_streaming;
    bool                                        m_validOverlay;
    TouchLayout                                 m_activeLayout;
    bool                                        m_yVisibility;
    bool                                        m_aEnabled;
    double                                      m_bOpacity;
    
    XTaskQueueRegistrationToken                 m_token;
    XTaskQueueHandle                            m_queue;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    DirectX::SpriteFont*                        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_localFont;
    std::unique_ptr<DirectX::SpriteFont>        m_remoteFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_circleTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        PrintLocalFont,
        PrintRemoteFont,
        ControllerFont,
        Touch,
        Background,
        Count,
    };
};
