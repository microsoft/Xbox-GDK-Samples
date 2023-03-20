//--------------------------------------------------------------------------------------
// SimpleExceptionHandling.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

void GenerateDump(EXCEPTION_POINTERS* exceptionPointers);

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
	    static std::wstring m_testOutputMessage;

private:

    enum ExceptionTest
    {
        e_handled,
        e_ignored,
        e_structured,
        e_vectored,
        e_language,
        e_recommended,
        e_null,
    };

    ExceptionTest m_lastTestRun;

    void ExecuteHandledException();
    void ExecuteIgnoredException();
    void ExecuteStructuredException();
    void ExecuteVectoredException();
    void ExecuteLanguageException();
    void ExecuteRecommended();
    void CatchLanguageExceptionWithStructured();

    void DrawHelpText(DirectX::XMFLOAT2& pos, ExceptionTest whichTest, const std::wstring& button);

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
