//--------------------------------------------------------------------------------------
// DevkitToolLauncher.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UI.h"

// The sample class.
// Command-line:
// DevkitToolLauncher.exe [-gpu/processUsesGpu] [-workingDir "path"] [-- "process" [commandline]]
class Sample final : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample(LPWSTR commandLine) noexcept(false);
    ~Sample();

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

    void ParseCommandLine(LPWSTR commandLine);

    // Input handling
    void UpdateInput(float elapsedSeconds);
    void SetButtonHoldTimer();
    void ResetButtonHoldTimer();
    bool IsButtonHoldTimerReady() const;

    // Rendering Suspend/Resume
    void SuspendRendering();
    void ResumeRendering();
    bool IsRenderingSuspended() const { return m_renderingSuspended; }

    // Tool process Management
    bool CreateToolProcessPipes();
    void CloseToolProcessPipes();
    void SpawnToolProcess(const std::string& processCommandLine, const std::string& workingDir, bool usesGpu);
    void TerminateToolProcess();
    bool IsToolProcessActive() const { return m_toolProcessActive; }
    void UpdateToolProcess();
    void ReadToolProcessPipes(bool flush = false);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    bool                                        m_renderingSuspended;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    float                                       m_buttonHoldTimerSeconds;
    float                                       m_buttonLongPressTimerSeconds;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UI implementation
    std::unique_ptr<ATG::UI>                    m_ui;

    // Command-line:
    // Set with [-gpu] or [-processUsesGpu]
    // If set, rendering in the sample will be suspended to allow the subprocesses
    // specified with m_processCommandLine to utilize the gpu.
    bool                                        m_clToolProcessUsesGpu;

    // Command-line:
    // Set with [-workingDir "path"]
    // This is passed to CreateProcess as the working directory for a subprocess specified with
    // the process command line.
    std::string                                 m_clToolProcessWorkingDir;

    // Command-line:
    // Set by specifying [--] on the the commandline. Everything after the "--" is interpreted as
    // the entire process command line for CreateProcess.
    std::string                                 m_clToolProcessCommandLine;

    // Work process data
    PROCESS_INFORMATION                         m_toolProcInfo;
    HANDLE                                      m_toolPipeStdOutRead;
    HANDLE                                      m_toolPipeStdOutWrite;
    HANDLE                                      m_toolPipeStdErrRead;
    HANDLE                                      m_toolPipeStdErrWrite;
    bool                                        m_toolProcessActive;
    bool                                        m_toolProcessUsesGpu;
};
