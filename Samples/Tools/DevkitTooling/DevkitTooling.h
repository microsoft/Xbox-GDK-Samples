//--------------------------------------------------------------------------------------
// DevkitTooling.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming(bool resetInput = true);
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return false; }

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

protected:

    // Input handling
    void UpdateInput();

    // Rendering Suspend/Resume
    void SuspendRendering();
    void ResumeRendering();
    bool IsRenderingSuspended() const { return m_renderingSuspended; }

    // Tool process Management
    bool CreateToolProcessPipes();
    void CloseToolProcessPipes();
    void SpawnToolProcess(const std::string& processCommandLine, const std::string& workingDir, bool usesGpu);
    void TerminateToolProcess(bool alreadyExited = false);
    bool IsToolProcessActive() const { return m_toolProcessActive; }
    void UpdateToolProcess();
    void ReadToolProcessPipes(bool flush = false);

    // Spawn CPU/GPU Tools
    void SpawnCPUTool();
    void SpawnGPUTool();

    // UI
    void InitializeUI();
    void Log(const char* text);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    bool                                            m_renderingSuspended;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;

    // UITK
    ATG::UITK::UIManager                            m_uiManager;
    ATG::UITK::UIInputState                         m_inputState;
    std::shared_ptr<ATG::UITK::UIConsoleWindow>     m_consoleWindow;

    // Work process data
    PROCESS_INFORMATION                             m_toolProcInfo;
    HANDLE                                          m_toolPipeStdOutRead;
    HANDLE                                          m_toolPipeStdOutWrite;
    HANDLE                                          m_toolPipeStdErrRead;
    HANDLE                                          m_toolPipeStdErrWrite;
    bool                                            m_toolProcessActive;
    bool                                            m_toolProcessUsesGpu;
};
