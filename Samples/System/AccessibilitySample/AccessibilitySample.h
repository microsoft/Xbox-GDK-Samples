//--------------------------------------------------------------------------------------
// AccessibilitySample.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "imgui.h"

#ifdef _GAMING_XBOX
    #include "imgui_impl_win32.h"
    #include "imgui_impl_gdk_dx12.h"
    #include "imgui_acc_gdk.h"
#else 
    #include "imgui_impl_dx12.h"
    #include "imgui_impl_win32.h"
    #include "imgui_acc_win32.h"
#endif

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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;


private:
    void Render();

    void Clear();

    void CreateDeviceDependentResources();


    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;


#ifdef _GAMING_XBOX
    imgui_acc_gdk* imgAccGdk;
#else
    imgui_acc_win32* imgAccWin32;
#endif
};
