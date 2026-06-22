//--------------------------------------------------------------------------------------
// FutexCombo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

class Sample;

namespace FutexTest
{
    void StartFutexTest(Sample* sample);
    void StopFutexTest();
}

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

    enum class MutexType
    {
        e_futex,
        e_slowtex,
        e_nulltex
    };

    enum class ContentionLevel
    {
        e_highContention,
        e_mediumContention,
        e_lowContention
    };

    void SetMutexTestResults(MutexType mutexType, ContentionLevel contentionLevel, uint32_t numThreads, bool threadsLocked, uint64_t workerWorkDone, uint64_t backgroundWorkDone, uint64_t* workerWorkDonePerThread, uint64_t* backgroundWorkDonePerThread);

private:

    // Sample code and data

    void CycleScreen();
    void DisplayLineHeader(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos);
    void DisplaySingleResults(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos, float* xCoords, uint32_t threadLockIndex);
    void DisplayMixedResults(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos, float* xCoords);
    void DrawGrid(DirectX::XMFLOAT2& pos, float* xCoords, uint32_t numGridLines);
    void DrawCenteredString(const wchar_t* str, float left, float right, float y);

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    enum class DisplayMode
    {
        e_lockedThreads,
        e_floatingThreads,
        e_mixedMode
    };

    DisplayMode                                 m_displayMode;
    double                                      m_nextCycleTimeSecs;
    std::mutex                                  m_resultsMutex;
    std::map<std::pair<MutexType, ContentionLevel>, std::pair<uint64_t, std::vector<uint64_t>>> m_workerWorkDone[2];        // 0 == affinity locked, 1 == floating
    std::map<std::pair<MutexType, ContentionLevel>, std::pair<uint64_t, std::vector<uint64_t>>> m_backgroundWorkDone[2];    // 0 == affinity locked, 1 == floating
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
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_gridBatch;
    std::unique_ptr<DirectX::BasicEffect>       m_gridBatchEffect;
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
