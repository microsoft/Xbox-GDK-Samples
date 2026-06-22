//--------------------------------------------------------------------------------------
// EnumerateThreads.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

class Sample;

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

    // Sample code and data

    void DrawCenteredString(const wchar_t* str, float left, float right, float y);

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    struct FoundThreadData
    {
        uint32_t m_priority;
        uint32_t m_threadID;
        std::wstring m_name;
        FoundThreadData() = default;
        FoundThreadData(const wchar_t* name, uint32_t threadID, uint32_t priority) noexcept :m_priority(priority), m_threadID(threadID), m_name(name) {}
    };

    std::vector<FoundThreadData> m_foundThreadsFromEnumeration;
    std::vector<std::thread*> m_testThreads;
    std::atomic<bool> m_killThreads;
    uint32_t m_lastThreadIndex;

    uint32_t ConvertThreadPriorityClassToBasePriority(int32_t priority, uint32_t priorityClass);
    void ThreadFunc(uint32_t index);
    void KillTestThreads();
    void CreateTestThreads(uint32_t numTotalThreads);
    bool EnumerateThreadsCallback(uint32_t processID, uint32_t threadID);
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
