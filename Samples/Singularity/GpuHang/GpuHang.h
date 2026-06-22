//--------------------------------------------------------------------------------------
// GpuHang.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "Hang.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void ParseCommandLine(const wchar_t* commandLine);
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
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons[DirectX::GamePad::MAX_PLAYER_COUNT];
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorHeap;

    // Desriptors for m_resourceDescriptorHeap
    struct ResourceDescriptors
    {
        enum : uint32_t
        {
            FontTitle,
            FontHangList,
            FontInstructions,
            FontController,
            FontDescription,

            Count
        };
    };

    // UI
    std::unique_ptr<DirectX::SpriteFont>            m_fontTitle;
    std::unique_ptr<DirectX::SpriteFont>            m_fontHangList;
    std::unique_ptr<DirectX::SpriteFont>            m_fontInstructions;
    std::unique_ptr<DirectX::SpriteFont>            m_fontController;
    std::unique_ptr<DirectX::SpriteFont>            m_fontDescription;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    float                                           m_scrollY;

    // User selections
    bool                                            m_hang;
    bool                                            m_takeCapture;
    uint32_t                                        m_queueType;
    uint32_t                                        m_hangAction;
    uint32_t                                        m_selectedHangIndex;
    IHang* SelectedHang() const { return Hang::HangList()[m_selectedHangIndex]; }

    // PIX capture
    std::wstring CaptureFileName() const;
    void BeginCapture() const;
    void EndCapture() const;
};
