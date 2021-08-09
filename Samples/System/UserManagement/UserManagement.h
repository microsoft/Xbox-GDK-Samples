//--------------------------------------------------------------------------------------
// UserManagement.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "GameInputDevice.h"
#include "GameUser.h"
#include "GameScreens.h"

#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample(LPWSTR lpCmdLine) noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Accessors to different management classes
    ATG::GameInputCollection* GetGameInputCollection() const;
    ATG::GameUserManager* GetGameUserManager() const;
    ATG::GameScreenManager* GetGameScreenManager() const;

    DX::DeviceResources* GetDeviceResources() const;
    // ATG::UITK::D3DResourcesProvider interface methods

    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }

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

    // User Management and Input
    std::unique_ptr<ATG::GameInputCollection>   m_gameInputCollection;
    std::unique_ptr<ATG::GameUserManager>       m_gameUserManager;

    // Game
    std::unique_ptr<ATG::GameScreenManager>     m_gameScreenManager;
    bool                                        m_crossRestartTriggered;

    // UITK objects
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_uiInputState;
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    
    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    enum Descriptors
    {
        Font18,
        Font24,
        Font48,
        ControllerFont,
        Count
    };
};
