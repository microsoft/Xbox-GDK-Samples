//--------------------------------------------------------------------------------------
// DevkitTooling.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DevkitTooling.h"

#include "ATGColors.h"
#include "StringUtil.h"

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

// Max size from CreateProcess() documentation.
constexpr size_t MAX_COMMANDLINE_SIZE = 32 * 1024;

Sample::Sample() noexcept(false)
    : m_renderingSuspended(false)
    , m_frame(0)
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

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
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

    InitializeUI();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

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

    // Update our UI input state and managed layout
    m_inputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_inputState);

    UpdateInput();

    UpdateToolProcess();
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

    m_uiManager.Render();

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
    m_deviceResources->Suspend();
}

void Sample::OnResuming(bool resetInput)
{
    m_deviceResources->Resume();

    if (resetInput)
    {
        m_timer.ResetElapsedTime();
        m_gamePadButtons.Reset();
        m_inputState.Reset();
    }
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Create the style renderer for the UI manager to use for rendering the UI scene styles
    auto os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}
#pragma endregion

#pragma region Input Handling
void Sample::UpdateInput()
{
    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
    {
        SpawnCPUTool();
    }
    else if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
    {
        SpawnGPUTool();
    }
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
        Log("Failed to create stdout pipe for redirection");
        return false;
    }
    result = CreatePipe(&m_toolPipeStdErrRead, &m_toolPipeStdErrWrite, &securityAttributes, 0);
    if (!result)
    {
        CloseHandle(m_toolPipeStdOutRead);
        CloseHandle(m_toolPipeStdOutWrite);
        m_toolPipeStdOutRead = INVALID_HANDLE_VALUE;
        m_toolPipeStdOutWrite = INVALID_HANDLE_VALUE;

        Log("Failed to create stdout pipe for redirection");
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
        Log(logString.c_str());

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
            Log(buf);

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

    Log("Process successfully started.");
}

void Sample::TerminateToolProcess(bool alreadyExited)
{
    if (!m_toolProcessActive)
    {
        return;
    }

    if (!alreadyExited)
    {
        Log("Terminating Process..");
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
        Log("Succeeded in Terminating Process");
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
            Log(logBuffer);

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

void Sample::ReadToolProcessPipes(bool flush)
{
    // StdOut
    static std::string pendingStringStdOut;
    ReadLinesFromPipe(m_toolPipeStdOutRead, pendingStringStdOut, flush,
        [&](const std::string& line)
        {
            Log(line.c_str());
        });

    // StdErr
    static std::string pendingStringStdErr;
    ReadLinesFromPipe(m_toolPipeStdErrRead, pendingStringStdErr, flush,
        [&](const std::string& line)
        {
            Log(line.c_str());
        });
}

void Sample::SpawnCPUTool()
{
    if (m_toolProcessActive)
    {
        return;
    }

    SpawnToolProcess("CPUTool.exe 5", ".", false);
}

void Sample::SpawnGPUTool()
{
    if (m_toolProcessActive)
    {
        return;
    }

    SpawnToolProcess("GPUTool.exe", ".", true);
}

#pragma region UI
void Sample::InitializeUI()
{
    auto root = m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/Layout.json");
    m_consoleWindow = root->GetChildById(ID("Output_Console_Window_Outer_Panel"))->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
}

void Sample::Log(const char* text)
{
    OutputDebugStringA(text);
    OutputDebugStringA(u8"\n");

    if (m_consoleWindow)
    {
        m_consoleWindow->AppendLineOfText(text);
    }
}
#pragma endregion
