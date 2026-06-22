//--------------------------------------------------------------------------------------
// FilePerfTestCombo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

    static Sample* GetInstance() { return s_instance; }

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    // Test control interface
    void ParseCommandLine(const wchar_t* commandlineParams);
    void SetWorkingOverlapDepth(uint32_t newQueueDepth) { m_workingOverlapDepth = newQueueDepth; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void PerformFileTasks();

    static Sample* s_instance;

    bool m_cmdLineError;
    bool m_doAsync;
    bool m_doSync;
    bool m_doZip;
    bool m_doRealtime;

    bool m_performFullSetup;
    bool m_performZipSetup;
    bool m_createPackedFile;
    uint32_t m_compressionRatio;

    bool m_doSyncDStorage;
    bool m_doAsyncDStorage;
    bool m_doSyncZipDStorage;
    bool m_doAsyncZipDStorage;

    bool m_usePackedFile;
    LoadOrder m_minLoadOrder, m_maxLoadOrder;
    DataSize m_minDataSize, m_maxDataSize;
    OverlapDepth m_minOverlapDepth, m_maxOverlapDepth;
    std::wstring m_directedFileName;
    std::vector<std::wstring> m_directedSectionName;

    std::wstring m_layoutFileName;

    uint32_t m_numIterations;

    std::atomic<uint32_t> m_testTypeRunning;
    std::atomic<bool> m_shutdownThread;
    std::atomic<bool> m_finishedTestRun;
    std::atomic<bool> m_creatingFlatFiles;
    std::atomic<bool> m_creatingZipFiles;
    std::atomic<uint32_t> m_workingLoadOrder;
    std::atomic<uint32_t> m_workingDataSize;
    std::atomic<uint32_t> m_workingOverlapDepth;

    std::atomic<uint32_t> m_finishedLoadOrder;
    std::atomic<uint32_t> m_finishedDataSize;
    std::atomic<uint32_t> m_finishedOverlapDepth;

    std::thread *m_workerThread;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame = 0;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>	m_background;
    std::unique_ptr<DirectX::SpriteFont>    m_regularFont;

    enum Descriptors
    {
        Background,
        RegularFont,
        Count
    };
};
