//--------------------------------------------------------------------------------------
// FrontPanelText.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "FrontPanel/RasterFont.h"
#include "FrontPanel/FrontPanelDisplay.h"
#include "FrontPanel/FrontPanelInput.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_background;

    enum Descriptors
    {
        Background,
        Count
    };

    // RasterFonts
    struct FontEntry
    {
        unsigned size;
        const wchar_t *name;
        ATG::RasterFont font;
    };
    unsigned FindFontEntry(const wchar_t *name, unsigned size, bool larger = true) const;
    unsigned                                        m_currentEntry;
    std::vector<FontEntry>                          m_fontEntries;

    // FrontPanel objects
    std::unique_ptr<ATG::FrontPanelDisplay>         m_frontPanelDisplay;
    std::unique_ptr<ATG::FrontPanelInput>           m_frontPanelInput;
    ATG::FrontPanelInput::ButtonStateTracker        m_frontPanelInputButtons;

    // It is only necessary to present to the front panel when there are changes to the pixels
    // We keep track of changes to front panel pixels using m_dirty
    bool                                            m_dirty;

    // Remember availability of the Front Panel
    bool                                            m_available;
};
