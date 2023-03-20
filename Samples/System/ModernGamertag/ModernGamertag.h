//--------------------------------------------------------------------------------------
// GlyphCacheCombo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "StringRenderer.h"

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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // GamerTag UI management
    HRESULT GetUserHandle(XUserAddOptions xUserAddOptions);
    HRESULT UpdateUserUIData();
    void SetDataDisplayable(bool value);

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

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_texture;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;

    std::unique_ptr<TextRenderer::StringShaper>               m_stringShaper;
    std::unique_ptr<TextRenderer::StringTextureAtlas>         m_stringTextureAtlas;
    std::unique_ptr<TextRenderer::StringRenderer>             m_stringRenderer;

    // User sign in.
    XUserHandle                                 m_userHandle;
    XTaskQueueHandle                            m_taskQueue;
    bool                                        m_userAddInProgress;

    // User GamerTag properties
    std::string                                 m_suggestedRendering;
    std::string                                 m_uniqueModernGamerTag;
    std::string                                 m_modernGamerTag;
    std::string                                 m_suffix;
    std::string                                 m_classicGamerTag;
    std::atomic_bool                            m_SetDataDisplayable;
};
