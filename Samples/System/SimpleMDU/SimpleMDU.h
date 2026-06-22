//--------------------------------------------------------------------------------------
// SimpleMDU.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "CompressedDataStream.h"
#include "InMemoryQueueHandler.h"
#include "FromStorageQueueHandler.h"


HRESULT HandleInMemoryDataChangeRequestAsync(XAsyncBlock* asyncBlock);
HRESULT HandleFromStorageDataChangeRequestAsync(XAsyncBlock* asyncBlock);

class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    void                    HandleInMemoryDataChangeRequest();
    void                    HandleFromStorageDataChangeRequest();

private:

    static constexpr uint32_t Ki = 1024;
    static constexpr uint32_t Mi = Ki * Ki;

    const float             c_randLimits[10]         = { 0.0f, 0.1f, 0.2f, 0.3f , 0.4f , 0.5f , 0.6f , 0.7f , 0.8f , 0.9f };
    const uint64_t          c_uniqueDataSize         = 64 * Mi;
    const uint64_t          c_dataSize               = 512 * Mi;    // Changing this to larger than a DWORD max means additional file write logic is required

    const uint16_t          c_inMemoryAlignments[2]  = { 16, 4096 };      
    const uint32_t          c_blockSizeOptions[12]   = { 16 * Ki, 32 * Ki, 64 * Ki, 128 * Ki, 256 * Ki, 512 * Ki, 1 * Mi, 2 * Mi, 4 * Mi, 8 * Mi, 16 * Mi, 32 * Mi };

    uint8_t                 m_selectedDataSet;
    uint8_t                 m_selectedInMemoryBlockSize;
    uint8_t                 m_selectedDataSetAlignment;

    InMemoryQueueHandler    m_inMemoryQueue;
    FromStorageQueueHandler m_fromStorageQueue;

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();


    void DisplayLineF(DirectX::XMFLOAT2& pos, DirectX::FXMVECTOR color, _In_z_ _Printf_format_string_ const wchar_t* format, ...);
    void DisplayLineF(DirectX::XMFLOAT2& pos, _In_z_ _Printf_format_string_ const wchar_t* format, ...);
    void DisplayLine(DirectX::XMFLOAT2& pos, DirectX::FXMVECTOR color, const wchar_t *text);
    void DisplayLine(DirectX::XMFLOAT2& pos, const wchar_t *text);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>	m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>		m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>		m_background;
    std::unique_ptr<DirectX::SpriteFont>		m_regularFont;
    std::unique_ptr<DirectX::SpriteFont>		m_largeFont;
    std::unique_ptr<DirectX::SpriteFont>		m_ctrlFont;

    enum Descriptors
    {
        Background,
        RegularFont,
        LargeFont,
        CtrlFont,
        Count
    };
};
