//--------------------------------------------------------------------------------------
// IntelligentDelivery.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "TextConsole.h"

// UITK
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

#include "UIChunk.h" 

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    void ConsoleWriteLine(const char* format, ...)
    {
        if (m_console)
        {
            va_list argList;
            va_start(argList, format);

            char buf[1024] = {};
            vsnprintf(buf, sizeof(buf), format, argList);
            m_console->WriteLine(DX::Utf8ToWide(buf).c_str());

            va_end(argList);
        }
    }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // UIStyleManager::D3DResourcesProvider interface methods
    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }

    void SetLegend(const char*);
    void HandlePressed(UIChunk*);
    void Enumerate(XPackageChunkSelectorType);
    void SetupTransferPackageProgress();
    void RefreshStatus();
    void CreateInstallationMonitor(UIChunk*, XPackageChunkSelector);
    void WatchChunkOrFeature(UIChunk*);
    
    std::map<std::string, std::unique_ptr<UIChunk>>                         m_UIFeatures;               // Features that make up the UI.
    std::map<uint16_t, std::unique_ptr<UIChunk>>                            m_UIChunks;                 // Chunks that make up the UI.

    char                                                                    m_packageIdentifier[XPACKAGE_IDENTIFIER_MAX_LENGTH];
    bool                                                                    m_packageIdentifierValid;
    XPackageInstallationMonitorHandle                                       m_pimHandleOverall;
    bool                                                                    m_pimHandleOverallActive;
    XTaskQueueHandle                                                        m_taskQueue;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                                    m_deviceResources;

    // Rendering loop timer.
    uint64_t                                                                m_frame;
    DX::StepTimer                                                           m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                                       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>                                      m_keyboard;
    std::unique_ptr<DirectX::Mouse>                                         m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                                m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>                                m_resourceDescriptors;

    enum Descriptors
    {
        Font,
        Reserve = 3,
        Count
    };

    // Text console
    std::unique_ptr<DX::TextConsole>                                        m_console;
    bool                                                                    m_showConsole;

    // UITK
    ATG::UITK::UIManager                                                    m_uiManager;
    ATG::UITK::UIInputState                                                 m_uiInputState;
};
