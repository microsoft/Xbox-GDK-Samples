//--------------------------------------------------------------------------------------
// DataBreakPoints.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "DataBreakpointTest.h"

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

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>	m_background;
    std::unique_ptr<DirectX::SpriteFont>    m_regularFont;
    std::unique_ptr<DirectX::SpriteFont>    m_largeFont;
    std::unique_ptr<DirectX::SpriteFont>    m_ctrlFont;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // Sample specific objects
    enum class TestStatus
    {
        TEST_NOT_RUN,
        TEST_SUCCESS,
        TEST_FAILURE,
    };
    DataBreakpointTest m_dataTest;
    TestStatus m_executionTestResult;
    TestStatus m_readTestResult;
    TestStatus m_readWriteTestResult;

    void DrawStatusString(const std::wstring& button, const std::wstring& testName, TestStatus status, DirectX::XMFLOAT2& pos);
    void DrawHelpText(DirectX::XMFLOAT2& pos, DataBreakpointTest::WhichTest);

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    enum Descriptors
    {
        Background,
        RegularFont,
        CtrlFont,
        Count
    };
};
