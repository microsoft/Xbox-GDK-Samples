//--------------------------------------------------------------------------------------
// SimpleDirectStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
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
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    enum testStatus
    {
        e_pending,
        e_success,
        e_failed,
        e_exception,
        e_notImplemented
    };

    // Sample code and data
    static const uint64_t c_ONE_MEG = 1024 * 1024;
    static const uint64_t c_NUM_MEGS_IN_GIG = 1024;
    static const uint32_t c_dataBlockSize = c_NUM_MEGS_IN_GIG * c_ONE_MEG;		// The data file is created in block, this is due to the uint32_t vs. uint64_t max value and available memory
    static const uint64_t c_dataFileSize = c_NUM_MEGS_IN_GIG * c_ONE_MEG * 5;	// this size file should give a decent amount of space for random loading. Couldn't hurt to increase this file size to increase the amount of seeking
    static const uint64_t c_zipFileSize = c_ONE_MEG * 10;                       // these are the requested size, it is still rounded up to c_dataBlockSize

    static const uint64_t c_realDataFileSize = (c_dataFileSize + (c_dataBlockSize - 1ULL)) & ~(c_dataBlockSize - 1ULL);		// round up to c_dataBlockSize
    static const std::wstring c_dataFileName;									// all the reads come from one data file
    static const std::wstring c_zipDataFileName;                                // all the zlib decompression reads come from one data file

    std::thread* m_dstorageSampleExecution;											// all the sample code runs in it's own thread to allow the rendering to run flat out and not impact performance
    void DSTORAGESampleFunc();														// Thread procedure for the sample code
    void CreateDataFiles();														// Checks for the data file and creates it if necessary, adds cost for first run, but zero for later runs

    void DisplayStatusLine(testStatus status, const wchar_t* sampleName, DirectX::XMFLOAT2& pos);

    std::atomic<testStatus> m_simpleLoadStatus;
    std::atomic<testStatus> m_cancellationStatus;
    std::atomic<testStatus> m_decompressionStatus;
    std::atomic<testStatus> m_inMemoryDecompressionStatus;
    std::atomic<testStatus> m_multipleQueuesStatus;
    std::atomic<testStatus> m_statusBatchStatus;
    std::atomic<testStatus> m_statusFenceStatus;
    std::atomic<testStatus> m_recommendedPatternStatus;
    std::atomic<testStatus> m_creatingDataFile;

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
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

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
