//--------------------------------------------------------------------------------------
// DevkitToolLauncher.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DevkitToolLauncher.h"

#include "ATGColors.h"
#include "StringUtil.h"
#include "CommandLine.h"
#include "RDTSCPStopWatch.h"

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

// Max size from CreateProcess() documentation.
constexpr size_t MAX_COMMANDLINE_SIZE = 32 * 1024;

Sample::Sample(LPWSTR commandLine) noexcept(false)
    : m_renderingSuspended(false)
    , m_frame(0)
    , m_buttonHoldTimerSeconds(0.0f)
    , m_buttonLongPressTimerSeconds(0.0f)
    , m_toolProcInfo{}
    , m_toolPipeStdOutRead(INVALID_HANDLE_VALUE)
    , m_toolPipeStdOutWrite(INVALID_HANDLE_VALUE)
    , m_toolPipeStdErrRead(INVALID_HANDLE_VALUE)
    , m_toolPipeStdErrWrite(INVALID_HANDLE_VALUE)
    , m_toolProcessActive(false)
    , m_toolProcessUsesGpu(false)
{
    m_toolProcInfo.hProcess = INVALID_HANDLE_VALUE;
    m_toolProcInfo.hThread = INVALID_HANDLE_VALUE;

    // Parse command-line
    ParseCommandLine(commandLine);

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    TerminateToolProcess();
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    auto const size = m_deviceResources->GetOutputSize();

    const bool toolProcessDesiredInCommandLine = m_clToolProcessCommandLine.size() > 0;

    // Create & initialize UI
    m_ui = std::make_unique<UI>();
    UI::Callbacks callbacks;
    callbacks.m_spawnToolProcessCallback = std::bind(&Sample::SpawnToolProcess, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    callbacks.m_isToolProcessActiveCallback = std::bind(&Sample::IsToolProcessActive, this);
    callbacks.m_getCommandLineProcessCallback =
        [&]()->std::string
        {
            return m_clToolProcessCommandLine;
        };
    callbacks.m_getCommandLineWorkingDirCallback =
        [&]()->std::string
        {
            return m_clToolProcessWorkingDir;
        };
    m_ui->Initialize(*this, size.right, size.bottom, toolProcessDesiredInCommandLine, callbacks);

    if (toolProcessDesiredInCommandLine)
    {
        SpawnToolProcess(m_clToolProcessCommandLine, m_clToolProcessWorkingDir, m_clToolProcessUsesGpu);
    }
}

// [-gpu/processUsesGpu] [-workingDir "path"] [-- "process" [commandline]]
void Sample::ParseCommandLine(LPWSTR commandLine)
{
    std::vector<std::wstring> parameters = BreakCommandLine_QuotedParameters(commandLine);

    // Parse parameters
    for (size_t index = 0; index < parameters.size(); ++index)
    {
        std::wstring param = DX::ToLower(parameters[index]);

        if (param.compare(L"--") == 0)
        {
            break;
        }
        else if (param.compare(L"-gpu") == 0 ||
            param.compare(L"-processusesgpu") == 0)
        {
            m_clToolProcessUsesGpu = true;
        }
        else if (param.compare(L"-workingdir") == 0 &&
            index < parameters.size() - 1)
        {
            m_clToolProcessWorkingDir = DX::WideToUtf8(parameters[index + 1]);
        }
    }

    // Parse application. Instead of reading parameters after the "--", take the entire rest of the string after it.
    // This allows any additional quoted parameters or other special cases to be passed-on properly.
    std::wstring commandLineToSearch = commandLine;
    size_t foundIndex = commandLineToSearch.find(L"--");
    if (foundIndex != std::wstring::npos)
    {
        // Skip any whitespace at the beginning of the string as that breaks CreateProcess()
        foundIndex += 2;
        while (foundIndex < commandLineToSearch.size() && isspace(commandLineToSearch[foundIndex]))
        {
            ++foundIndex;
        }
        m_clToolProcessCommandLine = DX::WideToUtf8(commandLineToSearch.substr(foundIndex));
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    RDTSCPStopWatch stopWatch;
    stopWatch.Start();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    // Sleep to give idle time back to sub-processes
    // Target is ~30FPS max
    const double elapsedSeconds = stopWatch.Stop();
    constexpr double targetSeconds = 1.0 / 30.0;
    const double secondsToSleep = targetSeconds - elapsedSeconds;
    if (secondsToSleep > 0.0)
    {
        const DWORD millisecondsToSleep = static_cast<DWORD>(secondsToSleep * 1000.0);
        if (millisecondsToSleep > 0)
        {
            Sleep(millisecondsToSleep);
        }
    }

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Update gamepad
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    UpdateInput(elapsedTime);

    UpdateToolProcess();

    m_ui->Update(elapsedTime);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    // Don't try to render if rendering is suspended.
    if (m_timer.GetFrameCount() == 0 ||
        IsRenderingSuspended())
    {
        return;
    }

    // We normally do this in Tick, but we do it here to implement the 'rendering suspend' behavior.
    m_deviceResources->WaitForOrigin();

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_ui->Render();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_ui->OnSuspending();

    m_deviceResources->Suspend();
}

void Sample::OnResuming(bool resetInput)
{
    m_deviceResources->Resume();

    if (resetInput)
    {
        m_timer.ResetElapsedTime();
        m_gamePadButtons.Reset();
        m_ui->OnResuming();
    }
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}
#pragma endregion

#pragma region Input Handling
void Sample::UpdateInput(float elapsedSeconds)
{
    if (m_buttonHoldTimerSeconds > 0.0f)
    {
        m_buttonHoldTimerSeconds -= elapsedSeconds;
    }

    // Navigation
    if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED ||
        (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavLeft);
        SetButtonHoldTimer();
    }
    else if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED ||
        (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavRight);
        SetButtonHoldTimer();
    }
    else if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED ||
        (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavUp);
        SetButtonHoldTimer();
    }
    else if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED ||
        (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavDown);
        SetButtonHoldTimer();
    }
    else if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED ||
        (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavPageUp);
        SetButtonHoldTimer();
    }
    else if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::ButtonState::PRESSED ||
        (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::HELD && IsButtonHoldTimerReady()))
    {
        m_ui->HandleInputCommand(UIInputCommand::NavPageDown);
        SetButtonHoldTimer();
    }

    // Select/Back
    if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
    {
        m_ui->HandleInputCommand(UIInputCommand::Select);
    }
    if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
    {
        m_ui->HandleInputCommand(UIInputCommand::Back);
    }

    if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
    {
        // Auto-scroll used by runtime. If in the browser, this command is ignored.
        m_ui->HandleInputCommand(UIInputCommand::ToggleAutoScroll);

        m_buttonLongPressTimerSeconds = 0.0f;
    }
    else if (m_gamePadButtons.y == GamePad::ButtonStateTracker::HELD)
    {
        m_buttonLongPressTimerSeconds += elapsedSeconds;
        if (m_buttonLongPressTimerSeconds >= 1.0f)
        {
            m_ui->HandleInputCommand(UIInputCommand::RefreshBrowser);
            m_buttonLongPressTimerSeconds = FLT_MIN;
        }
    }
    else if (m_gamePadButtons.y == GamePad::ButtonStateTracker::RELEASED)
    {
        m_buttonLongPressTimerSeconds = 0.0f;
    }

    // If a controller holds down 'b' while a work process is active, it will be terminated
    if (m_toolProcessActive)
    {
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_buttonLongPressTimerSeconds = 0.0f;
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::HELD)
        {
            m_buttonLongPressTimerSeconds += elapsedSeconds;
            if (m_buttonLongPressTimerSeconds >= 1.0f)
            {
                TerminateToolProcess();
                m_buttonLongPressTimerSeconds = FLT_MIN;
            }
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::RELEASED)
        {
            m_buttonLongPressTimerSeconds = 0.0f;
        }
    }
}

void Sample::SetButtonHoldTimer()
{
    m_buttonHoldTimerSeconds = 0.2f;
}

void Sample::ResetButtonHoldTimer()
{
    m_buttonHoldTimerSeconds = 0.0f;
}

bool Sample::IsButtonHoldTimerReady() const
{
    return m_buttonHoldTimerSeconds <= 0.0f;
}
#pragma endregion

void Sample::SuspendRendering()
{
    if (!m_renderingSuspended)
    {
        OnSuspending();
        m_renderingSuspended = true;
    }
}

void Sample::ResumeRendering()
{
    if (m_renderingSuspended)
    {
        OnResuming(false);
        m_renderingSuspended = false;
    }
}

bool Sample::CreateToolProcessPipes()
{
    SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES) };
    securityAttributes.bInheritHandle = TRUE;
    BOOL result = CreatePipe(&m_toolPipeStdOutRead, &m_toolPipeStdOutWrite, &securityAttributes, 0);
    if (!result)
    {
        m_ui->Log("Failed to create stdout pipe for redirection");
        return false;
    }
    result = CreatePipe(&m_toolPipeStdErrRead, &m_toolPipeStdErrWrite, &securityAttributes, 0);
    if (!result)
    {
        CloseHandle(m_toolPipeStdOutRead);
        CloseHandle(m_toolPipeStdOutWrite);
        m_toolPipeStdOutRead = INVALID_HANDLE_VALUE;
        m_toolPipeStdOutWrite = INVALID_HANDLE_VALUE;

        m_ui->Log("Failed to create stderr pipe for redirection");
        return false;
    }

    return true;
}

void Sample::CloseToolProcessPipes()
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

void Sample::SpawnToolProcess(const std::string& processCommandLine, const std::string& workingDir, bool usesGpu)
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
        STARTUPINFOA startupInfo = { sizeof(STARTUPINFOA) };
        // STARTF_USESTDHANDLES is not currently defined withing WINAPI_FAMILY_GAMES,
        // but needs to be set to enable the subprocess inheriting hStdInput/hStrError/hStdOutput
        // from the startupInfo.
        startupInfo.dwFlags = /*STARTF_USESTDHANDLES*/0x00000100;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdError = m_toolPipeStdErrWrite;
        startupInfo.hStdOutput = m_toolPipeStdOutWrite;

        const std::string logString = "Starting process " + processCommandLine;
        m_ui->Log(logString.c_str());

        // Create process. Needs an editable buffer for the process command line.
        std::vector<char> commandLine(MAX_COMMANDLINE_SIZE, 0);
        sprintf_s(commandLine.data(), MAX_COMMANDLINE_SIZE, "%s", processCommandLine.c_str());
        m_toolProcessActive = CreateProcessA(
            NULL,
            commandLine.data(),
            NULL,
            NULL,
            true,
            CREATE_NO_WINDOW,
            NULL,
            workingDir.c_str(),
            &startupInfo,
            &m_toolProcInfo
        );

        if (!m_toolProcessActive)
        {
            char buf[1024] = {};
            DWORD lastError = GetLastError();
            sprintf_s(buf, 1024, "Failed to create process. GetLastError=%d", lastError);
            m_ui->Log(buf);

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

    m_ui->Log("Process successfully started.");
}

void Sample::TerminateToolProcess()
{
    if (!m_toolProcessActive)
    {
        return;
    }

    // Determine if tool process has already exited or not
    bool alreadyExited = true;
    if (DX::safe_handle(m_toolProcInfo.hProcess))
    {
        alreadyExited = false;
        DWORD exitCode = 0;
        if (GetExitCodeProcess(m_toolProcInfo.hProcess, &exitCode) &&
            exitCode != STILL_ACTIVE)
        {
            alreadyExited = true;
        }
    }

    if (!alreadyExited)
    {
        m_ui->Log("Terminating Process..");
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
        m_ui->Log("Succeeded in Terminating Process");
    }

    // Ensure rendering is active when it needs to be
    if (IsRenderingSuspended())
    {
        ResumeRendering();
    }
}

void Sample::UpdateToolProcess()
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
            m_ui->Log(logBuffer);

            TerminateToolProcess();
        }
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

void Sample::ReadToolProcessPipes(bool flush)
{
    // StdOut
    static std::string pendingStringStdOut;
    ReadLinesFromPipe(m_toolPipeStdOutRead, pendingStringStdOut, flush,
        [&](const std::string& line)
        {
            std::string toLog = "[stdout] " + line;
            m_ui->Log(toLog.c_str());
        });

    // StdErr
    static std::string pendingStringStdErr;
    ReadLinesFromPipe(m_toolPipeStdErrRead, pendingStringStdErr, flush,
        [&](const std::string& line)
        {
            std::string toLog = "[stderr] " + line;
            m_ui->Log(toLog.c_str());
        });
}
