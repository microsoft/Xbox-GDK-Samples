//--------------------------------------------------------------------------------------
// GpuBench.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void ParseCommandLine(const wchar_t* commandLine);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void RenderUI();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void OpenLogFile(bool reset);
    void CloseLogFile();
    void Output(const wchar_t* str, size_t len) const;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons[DirectX::GamePad::MAX_PLAYER_COUNT];

    struct ResourceDescriptors
    {
        enum : uint32_t
        {
            FontBenchmark,
            FontReport,
            FontController, 

            Count
        };
    };

    // UI
    std::unique_ptr<DirectX::SpriteFont>            m_fontBenchmark;
    std::unique_ptr<DirectX::SpriteFont>            m_fontReport;
    std::unique_ptr<DirectX::SpriteFont>            m_fontController;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    float                                           m_scrollY;

    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;

    // Version check
    bool                                            m_versionCheck;

    // Log file
    const wchar_t*                                  m_logFileName;
    FILE*                                           m_logFile;

    // Benchmarks
    static constexpr uint32_t                       m_invalidBenchmarkIndex = uint32_t(-1);
    std::queue<uint32_t>                            m_pendingBenchmarkIndices;
    uint32_t                                        m_runningBenchmarkIndex;
    uint32_t                                        m_selectedBenchmarkIndex;
    IBenchmark* RunningBenchmark() const {return (m_invalidBenchmarkIndex == m_runningBenchmarkIndex) ? nullptr : Benchmark::BenchmarkList()[m_runningBenchmarkIndex];}
    IBenchmark* SelectedBenchmark() const {return Benchmark::BenchmarkList()[m_selectedBenchmarkIndex];}
    void StartBenchmark(uint32_t benchmarkIndex);
    void StopBenchmark();
};
