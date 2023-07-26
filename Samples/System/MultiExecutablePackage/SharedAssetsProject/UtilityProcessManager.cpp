//--------------------------------------------------------------------------------------
// UtilityProcessManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UtilityProcessManager.h"
#include "OSHelpers.h"

using namespace DX;

// Max size from CreateProcess() documentation.
constexpr size_t MAX_COMMANDLINE_SIZE = 32 * 1024;

UtilityProcessManager::UtilityProcessManager(UITKLogManager* logger) noexcept(false)
    : m_renderingSuspended(false)
    , m_toolProcInfo{}
    , m_toolPipeStdOutRead(INVALID_HANDLE_VALUE)
    , m_toolPipeStdOutWrite(INVALID_HANDLE_VALUE)
    , m_toolPipeStdErrRead(INVALID_HANDLE_VALUE)
    , m_toolPipeStdErrWrite(INVALID_HANDLE_VALUE)
    , m_toolProcessActive(false)
    , m_toolProcessUsesGpu(false)
    , m_logger(logger)
{
    m_toolProcInfo.hProcess = INVALID_HANDLE_VALUE;
    m_toolProcInfo.hThread = INVALID_HANDLE_VALUE;
}

UtilityProcessManager::~UtilityProcessManager()
{
    TerminateToolProcess();
}

#pragma region ExtraProcess
bool UtilityProcessManager::CreateToolProcessPipes()
{
    SECURITY_ATTRIBUTES securityAttributes;
    ZeroMemory(&securityAttributes, sizeof(SECURITY_ATTRIBUTES));
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);

    securityAttributes.bInheritHandle = TRUE;
    BOOL result = CreatePipe(&m_toolPipeStdOutRead, &m_toolPipeStdOutWrite, &securityAttributes, 0);
    if (!result)
    {
        m_logger->Log("Failed to create stdout pipe for redirection");
        return false;
    }
    result = CreatePipe(&m_toolPipeStdErrRead, &m_toolPipeStdErrWrite, &securityAttributes, 0);
    if (!result)
    {
        CloseHandle(m_toolPipeStdOutRead);
        CloseHandle(m_toolPipeStdOutWrite);
        m_toolPipeStdOutRead = INVALID_HANDLE_VALUE;
        m_toolPipeStdOutWrite = INVALID_HANDLE_VALUE;

        m_logger->Log("Failed to create stdout pipe for redirection");
        return false;
    }

    return true;
}

void UtilityProcessManager::CloseToolProcessPipes()
{
    if (DX::safe_handle(m_toolPipeStdOutRead))
    {
        CloseHandle(m_toolPipeStdOutRead);
        m_toolPipeStdOutRead = INVALID_HANDLE_VALUE;
    }
    if (DX::safe_handle(m_toolPipeStdOutWrite))
    {
        CloseHandle(m_toolPipeStdOutWrite);
        m_toolPipeStdOutWrite = INVALID_HANDLE_VALUE;
    }
    if (DX::safe_handle(m_toolPipeStdErrRead))
    {
        CloseHandle(m_toolPipeStdErrRead);
        m_toolPipeStdErrRead = INVALID_HANDLE_VALUE;
    }
    if (DX::safe_handle(m_toolPipeStdErrWrite))
    {
        CloseHandle(m_toolPipeStdErrWrite);
        m_toolPipeStdErrWrite = INVALID_HANDLE_VALUE;
    }
}

void UtilityProcessManager::SpawnToolProcess(const std::string& processCommandLine, const std::string& workingDir, bool usesGpu)
{
    if (m_toolProcessActive)
    {
        return;
    }

    // If the process to spawn is a gpu process, then suspend rendering
    m_toolProcessUsesGpu = usesGpu;
    if (usesGpu)
    {
        SuspendRendering();
    }

    // Create new pipes for stdout and stderr redirection
    if (!CreateToolProcessPipes())
    {
        return;
    }

    // Try to spawn process with redirected stdout
    {
        STARTUPINFOA startupInfo;
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);

        // STARTF_USESTDHANDLES is not currently defined withing WINAPI_FAMILY_GAMES,
        // but needs to be set to enable the subprocess inheriting hStdInput/hStrError/hStdOutput
        // from the startupInfo.
        startupInfo.dwFlags = /*STARTF_USESTDHANDLES*/0x00000100;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdError = m_toolPipeStdErrWrite;
        startupInfo.hStdOutput = m_toolPipeStdOutWrite;

        const std::string logString = "Starting process " + processCommandLine;
        m_logger->Log(logString.c_str());

        // Create process. Needs an editable buffer for the process command line.
        char* commandLine = new char[MAX_COMMANDLINE_SIZE];
        sprintf_s(commandLine, MAX_COMMANDLINE_SIZE, "%s", processCommandLine.c_str());
        m_toolProcessActive = CreateProcessA(
            NULL,
            commandLine,
            NULL,
            NULL,
            true,
            CREATE_NO_WINDOW,
            NULL,
            workingDir.c_str(),
            &startupInfo,
            &m_toolProcInfo
        );
        delete[] commandLine;

        if (!m_toolProcessActive)
        {
            char buf[1024] = {};
            DWORD lastError = GetLastError();
            sprintf_s(buf, 1024, "Failed to create process. GetLastError=%d", lastError);
            m_logger->Log(buf);

            // Close handles in failure
            if (DX::safe_handle(m_toolProcInfo.hThread))
            {
                CloseHandle(m_toolProcInfo.hThread);
                m_toolProcInfo.hThread = INVALID_HANDLE_VALUE;
            }
            if (DX::safe_handle(m_toolProcInfo.hProcess))
            {
                CloseHandle(m_toolProcInfo.hProcess);
                m_toolProcInfo.hProcess = INVALID_HANDLE_VALUE;
            }

            CloseToolProcessPipes();

            return;
        }
    }

    m_logger->Log("Process successfully started.");
}

void UtilityProcessManager::TerminateToolProcess(bool alreadyExited)
{
    if (!m_toolProcessActive)
    {
        return;
    }

    if (!alreadyExited)
    {
        m_logger->Log("Terminating Process..");
    }

    // Flush the rest of the logging from the process pipes
    ReadToolProcessPipes(true);

    // Ensure process is terminated (if needed) and close handles
    if (DX::safe_handle(m_toolProcInfo.hThread))
    {
        CloseHandle(m_toolProcInfo.hThread);
        m_toolProcInfo.hThread = INVALID_HANDLE_VALUE;
    }
    if (DX::safe_handle(m_toolProcInfo.hProcess))
    {
        if (!alreadyExited)
        {
            TerminateProcess(m_toolProcInfo.hProcess, 0);
        }
        std::ignore = WaitForSingleObject(m_toolProcInfo.hProcess, INFINITE);
        CloseHandle(m_toolProcInfo.hProcess);
        m_toolProcInfo.hProcess = INVALID_HANDLE_VALUE;
    }

    // Close pipe handles
    CloseToolProcessPipes();

    m_toolProcessActive = false;

    if (!alreadyExited)
    {
        m_logger->Log("Succeeded in Terminating Process");
    }
}

void UtilityProcessManager::UpdateToolProcess()
{
    // Watch the work process
    if (m_toolProcessActive)
    {
        ReadToolProcessPipes();

        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_toolProcInfo.hProcess, &exitCode) &&
            exitCode != STILL_ACTIVE)
        {
            char logBuffer[512] = {};
            sprintf_s(logBuffer, 512, u8"Work process exited with code %d", exitCode);
            m_logger->Log(logBuffer);

            TerminateToolProcess(true);
        }
    }

    // Ensure rendering is active when it needs to be
    if (IsRenderingSuspended() &&
        (!m_toolProcessActive || !m_toolProcessUsesGpu))
    {
        ResumeRendering();
    }
}

// Attempts to read lines from the passed-in pipe. Each completed line has the lineHandler called with it.
// Needs a passed-in string to cache incomplete lines in.
void ReadLinesFromPipe(HANDLE pipeHandle, std::string& lineCache, bool flush, std::function<void(const std::string& line)> lineHandler)
{
    DWORD bytesRead = 0;
    const DWORD bufferSize = 2048;
    char readBuffer[bufferSize] = {};

    while (true)
    {
        DWORD bytesAvailable = 0;
        PeekNamedPipe(pipeHandle, NULL, 0, NULL, &bytesAvailable, NULL);
        if (bytesAvailable > 0)
        {
            if (ReadFile(pipeHandle, readBuffer, bufferSize - 1, &bytesRead, NULL) &&
                bytesRead > 0)
            {
                readBuffer[bytesRead] = '\0';
                std::stringstream stream(readBuffer);
                std::string readLine;
                while (std::getline(stream, readLine, '\n'))
                {
                    lineCache += readLine;

                    if (!stream.eof())
                    {
                        lineHandler(lineCache);
                        lineCache.clear();
                    }
                }
            }
        }

        if (flush)
        {
            if (bytesAvailable == 0)
            {
                if (lineCache.size() > 0)
                {
                    lineHandler(lineCache);
                    lineCache.clear();
                }

                break;
            }

            // Continue to another loop if there were bytes available
        }
        else
        {
            break;
        }
    }
}

void UtilityProcessManager::ReadToolProcessPipes(bool flush)
{
    // StdOut
    static std::string pendingStringStdOut;
    ReadLinesFromPipe(m_toolPipeStdOutRead, pendingStringStdOut, flush,
        [&](const std::string& line)
        {
            m_logger->Log(line.c_str());
        });

    // StdErr
    static std::string pendingStringStdErr;
    ReadLinesFromPipe(m_toolPipeStdErrRead, pendingStringStdErr, flush,
        [&](const std::string& line)
        {
            m_logger->Log(line.c_str());
        });
}

void UtilityProcessManager::SpawnCPUTool()
{
    if (m_toolProcessActive)
    {
        return;
    }

    SpawnToolProcess("CPUTool.exe 5", ".", false);
}


#pragma endregion

void UtilityProcessManager::SuspendRendering()
{
    m_renderingSuspended = true;
}

void UtilityProcessManager::ResumeRendering()
{
    m_renderingSuspended = false;
}
