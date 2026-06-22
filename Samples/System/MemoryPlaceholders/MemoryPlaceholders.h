//--------------------------------------------------------------------------------------
// MemoryPlaceholders.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "TitleReservedMemory.h"
#include "PlaceholderAllocations.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    enum class MemoryPlaceholderDemos
    {
        TITLE_RESERVED_ALLOC,
        TITLE_RESERVED_FREE,
        PLACE_HOLDER_ALLOC,
        PLACE_HOLDER_FREE,
    };
    static constexpr size_t c_defaultCPUPlaceholderSize = 2ULL * 1024 * 1024 * 1024 * 1024;
    static constexpr size_t c_defaultGPUPlaceholderSize = 10ULL * 1024 * 1024 * 1024;
    static constexpr size_t c_minAllocationSize = 4096;
    static constexpr size_t c_maxAllocationSize = 100ULL * 1024 * 1024;
    ATG::PlaceholderRegion m_cpuPlaceholder;
    ATG::PlaceholderRegion m_gpuPlaceholder;
    std::vector<void *> m_titleReservedRangeAllocations;
    std::vector<void *> m_cpuPlaceholderAllocations;
    std::vector<void *> m_gpuPlaceholderAllocations;

    void AllocateFromTitleReservedRange();
    void ReleaseToTitleReservedRange();
    void AllocateFromPlaceholder(bool graphics);
    void ReleaseToPlaceholder(bool graphics);
    void GetRandomAllocationParameters(bool graphics, size_t& size, uint32_t& allocationType, uint64_t& xmemFlags);

    void DrawStatusString(const std::wstring& button, const std::wstring& testName, DirectX::XMFLOAT2& pos);

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
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

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
