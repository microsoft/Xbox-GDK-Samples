//--------------------------------------------------------------------------------------
// UtilityProcessManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "UITKLogManager.h"

class UtilityProcessManager
{
public:
    UtilityProcessManager(UITKLogManager* logger) noexcept(false);
    ~UtilityProcessManager();

    UtilityProcessManager(UtilityProcessManager&&) = default;
    UtilityProcessManager& operator= (UtilityProcessManager&&) = delete;

    UtilityProcessManager(UtilityProcessManager const&) = delete;
    UtilityProcessManager& operator= (UtilityProcessManager const&) = delete;

    // Tool process Management
    bool CreateToolProcessPipes();
    void CloseToolProcessPipes();
    void SpawnToolProcess(const std::string& processCommandLine, const std::string& workingDir, bool usesGpu);
    void TerminateToolProcess(bool alreadyExited = false);
    bool IsToolProcessActive() const { return m_toolProcessActive; }
    void UpdateToolProcess();
    void ReadToolProcessPipes(bool flush = false);

    // Rendering Suspend/Resume
    void SuspendRendering();
    void ResumeRendering();
    bool IsRenderingSuspended() const { return m_renderingSuspended; }

    // Spawn CPU/GPU Tools
    void SpawnCPUTool();

private:


    // Device Resources
    bool                                            m_renderingSuspended;

    // Work process data
    PROCESS_INFORMATION                             m_toolProcInfo;
    HANDLE                                          m_toolPipeStdOutRead;
    HANDLE                                          m_toolPipeStdOutWrite;
    HANDLE                                          m_toolPipeStdErrRead;
    HANDLE                                          m_toolPipeStdErrWrite;
    bool                                            m_toolProcessActive;
    bool                                            m_toolProcessUsesGpu;

    // Logging set up
    UITKLogManager*                                 m_logger;
};

