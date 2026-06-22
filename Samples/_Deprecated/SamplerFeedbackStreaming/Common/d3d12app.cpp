//--------------------------------------------------------------------------------------
// D3D12App.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "D3D12App.hpp"

#if DEVTEST_USE_XINPUT
#include "win32app.h"
#endif


#if defined(_XBOX_ONE) && !defined(NO_SCREEN_GRAB)
#define SCREEN_GRAB_INCLUDED
#include "..\ScreenGrab\ScreenGrab12.h"
#endif

#if UWP_BUILD
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::Foundation::Collections;
#if !defined(_XBOX_ONE)
using namespace Windows::Graphics::Display;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Input;
#endif
#endif

static D3D12App* pApp = nullptr;

// PIX event colors
const DWORD EVT_COLOR_FRAME = PIX_COLOR_INDEX(1);
const DWORD EVT_COLOR_UPDATE = PIX_COLOR_INDEX(2);
const DWORD EVT_COLOR_RENDER = PIX_COLOR_INDEX(3);

D3D12App::D3D12App()
{
    pApp = this;

    ZeroMemory( &m_InitParams, sizeof(m_InitParams) );

    m_WaitForDebuggerAttach = false;
    m_FrameIndex = 0;
    
    m_pd3dDevice = nullptr;
    m_pd3dCmdQueue = nullptr;
    m_pCmdAllocator = nullptr;
    m_pGpuFence = nullptr;
    m_pDefaultRootSignature = nullptr;
    m_pFrameCmdAllocator = nullptr;
    ZeroMemory(m_pFrameCmdAllocatorPool, sizeof(m_pFrameCmdAllocatorPool));
    m_CpuFence = 0;
    ZeroMemory(&m_SelectedAdapterDesc, sizeof(m_SelectedAdapterDesc));

    m_RenderingPaused = false;
    m_ShutdownRequested = false;

    ZeroMemory(&m_LastReading, sizeof(m_LastReading));
    m_PressedButtons = 0;
    m_ThumbLeftX = 0.0f;
    m_ThumbLeftY = 0.0f;
    m_ThumbRightX = 0.0f;
    m_ThumbRightY = 0.0f;
    m_TranslatedKeyButtons = 0;
    m_TranslatedStickButtons = 0;
    m_MouseDeltaX = 0;
    m_MouseDeltaY = 0;
    m_InvertMouseY = false;
    m_EmulateLeftStickFromKeyboard = false;

#if !defined(_GAMING_XBOX)
    m_pSwapChain = nullptr;
#endif
    ZeroMemory(m_RenderTargetView, sizeof(m_RenderTargetView));
    ZeroMemory(m_pRenderTargetTexture, sizeof(m_pRenderTargetTexture));
    m_pDepthStencilTexture = nullptr;
    ZeroMemory(&m_Viewport, sizeof(m_Viewport));
    ZeroMemory(&m_ScissorRect, sizeof(m_ScissorRect));

    m_AspectRatio = 0.0f;

#if defined(_GAMING_XBOX)
    // GFX9 driver only translates packets in instrumented builds so don't try retail driver -- it just won't work
    m_InitParams.CreateDeviceParameters.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;//NONE;
#if defined(_DEBUG) || defined(DBG)
    m_InitParams.CreateDeviceParameters.ProcessDebugFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_DEBUG;
#endif
#else
#if defined(_DEBUG) || defined(DBG)
    m_InitParams.CreateDebugDevice = TRUE;
#else
    m_InitParams.CreateDebugDevice = FALSE;
#endif
#endif
}

void D3D12App::CloseApp()
{
    m_ShutdownRequested = true;
    m_TitleRunning = false;
#if UWP_BUILD
    CoreApplication::Exit();
#endif
}

#if DEVTEST_USE_HWND
HWND D3D12App::s_hWnd = nullptr;
HANDLE D3D12App::s_plmSuspendComplete = nullptr;
HANDLE D3D12App::s_plmSignalResume = nullptr;

LRESULT CALLBACK D3D12App::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (pApp != nullptr)
        {
            pApp->WindowResize(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_DESTROY:
        if (pApp)
        {
            pApp->m_TitleRunning = false;
        }
        PostQuitMessage(0);
        break;

    case WM_USER:
        if (pApp)
        {
            pApp->SuspendRendering();

            // Complete deferral
            SetEvent(pApp->s_plmSuspendComplete);

            (void)WaitForSingleObject(pApp->s_plmSignalResume, INFINITE);

            pApp->ResumeRendering();
        }
        break;

#if DEVTEST_USE_HWND && !defined(_GAMING_XBOX)
    case WM_KEYDOWN:
    case WM_KEYUP:
        TranslateKey(pApp, message, wParam, lParam);
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        TranslateMouse(pApp, hWnd, message, wParam, lParam);
        break;
#endif

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
#endif


HRESULT D3D12App::CreateWithoutAppModel(const char* pAppName)
{
    m_pAppName = pAppName;

    WindowCreated();

    return S_OK;
}

#if DEVTEST_USE_HWND
HRESULT D3D12App::CreateWin32Window()
{
#if defined(_GAMING_DESKTOP) || PC_BUILD
    SetProcessDPIAware();
#endif

    // Register window class
    const WCHAR* strClassName = L"D3D12_Devtest";
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = D3D12App::WndProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = strClassName;
    if (!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }

    const DWORD windowStyle = WS_OVERLAPPEDWINDOW;

    RECT rc;
    rc.bottom = m_WindowBoundsHeight;
    rc.top = 0;
    rc.left = 0;
    rc.right = m_WindowBoundsWidth;
    AdjustWindowRectEx(&rc, windowStyle, FALSE, 0);

    s_hWnd = CreateWindowEx(0, strClassName, m_Init.strWindowTitle, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, m_WindowBoundsWidth, m_WindowBoundsHeight, nullptr, nullptr, nullptr, nullptr);
    if (!s_hWnd)
    {
        return E_FAIL;
    }

#if defined(_GAMING_DESKTOP) || PC_BUILD
    ShowWindow(s_hWnd, SW_SHOWNORMAL);
#endif

    s_plmSuspendComplete = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    s_plmSignalResume = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    if (!s_plmSuspendComplete || !s_plmSignalResume)
    {
        return E_FAIL;
    }

    return S_OK;
}
#endif

HRESULT D3D12App::RunWithoutAppModel()
{
    // Initialize sample
    HRESULT hr = FrameworkInitialize();
    DX::ThrowIfFailed( hr );

    hr = Initialize();
    DX::ThrowIfFailed( hr );

#if defined(_GAMING_XBOX) && DEVTEST_USE_HWND
    PAPPSTATE_REGISTRATION hPLM = {};
    if (RegisterAppStateChangeNotification([](BOOLEAN quiesced, PVOID context)
    {
        if (quiesced)
        {
            ResetEvent(D3D12App::s_plmSuspendComplete);
            ResetEvent(D3D12App::s_plmSignalResume);

            // To ensure we use the main UI thread to process the notification, we self-post a message
            PostMessage(reinterpret_cast<HWND>(context), WM_USER, 0, 0);

            // To defer suspend, you must wait to exit this callback
            (void)WaitForSingleObject(D3D12App::s_plmSuspendComplete, INFINITE);
        }
        else
        {
            SetEvent(D3D12App::s_plmSignalResume);
        }
    }, D3D12App::s_hWnd, &hPLM))
        return 1;
#endif

    while (m_TitleRunning)
    {
        MSG msg = { nullptr, WM_NULL };

    #if DEVTEST_USE_HWND
        // pump win32 messages
        while (PeekMessage(&msg, nullptr, 0, 0, TRUE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    #endif

        if (m_TitleRunning)
        {
            m_TitleRunning = Tick();
        }
    }

    WindowClosing();

#if defined(_GAMING_XBOX) && DEVTEST_USE_HWND
    UnregisterAppStateChangeNotification(hPLM);

    CloseHandle(s_plmSuspendComplete);
    CloseHandle(s_plmSignalResume);
#endif

    return S_OK;
}

void D3D12App::WaitForDebuggerAttach()
{
    while (!IsDebuggerPresent())
    {
        Sleep(250);
    }
    __debugbreak();
}

void D3D12App::ParseCommandLine(const wchar_t* lpCmdLine)
{
    wchar_t strCopy[MAX_PATH];
    wcscpy_s(strCopy, lpCmdLine);

    wchar_t *nextToken;
    wchar_t *token = wcstok_s(strCopy, L" ", &nextToken);
    if (token != nullptr)
    {
        int argc = 0;
        const wchar_t* argv[MAX_PATH];

        while (token != nullptr && argc < MAX_PATH)
        {
            argv[argc++] = token;
            token = wcstok_s(nullptr, L" ", &nextToken);
        }

        ParseTokenizedCommandLine(argc, argv);
    }
}

void D3D12App::ParseTokenizedCommandLine(int argc, const wchar_t** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        const wchar_t* strCommandLine = argv[i];

        if (wcsstr(strCommandLine, L"-vsdebug") != nullptr)
        {
            m_WaitForDebuggerAttach = true;
        }

        if (wcsstr(strCommandLine, L"-validated") != nullptr || wcsstr(strCommandLine, L"-debug") != nullptr)
        {
#if defined(_XBOX_ONE) && defined(_TITLE)
            m_InitParams.CreateDeviceParameters.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_VALIDATED;
#else
            m_InitParams.CreateDebugDevice = TRUE;
#endif
        }

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        if (wcsstr(strCommandLine, L"-instrumented") != nullptr)
        {
            m_InitParams.CreateDeviceParameters.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
        }
#endif

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
        if (wcsstr(strCommandLine, L"-retail") != nullptr)
        {
            m_InitParams.CreateDeviceParameters.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_NONE;
        }
#endif

        const wchar_t* pixCaptureAtFrame = wcsstr(strCommandLine, L"-PixCaptureAtFrame");
        if (pixCaptureAtFrame == nullptr)
        {
            pixCaptureAtFrame = wcsstr(strCommandLine, L"/PixCaptureAtFrame");
        }
        if (pixCaptureAtFrame != nullptr)
        {
            wchar_t strCopy[MAX_PATH];
            wcscpy_s(strCopy, pixCaptureAtFrame);

            m_pixCapture = true;
            m_frameCountdown = 30;

            wchar_t *next_token = nullptr;
            wchar_t* token = wcstok_s(strCopy, L"=", &next_token);
            if (token != nullptr)
            {
                token = wcstok_s(nullptr, L" ", &next_token);
                if (token != nullptr)
                {
                    m_frameCountdown = _wtoi(token);
                }
            }
        }

        if (wcsstr(strCommandLine, L"screenshot"))
        {
            m_screenshot = true;
            m_frameCountdown = 30;
        }
    }
}

#if UWP_BUILD
void D3D12App::WindowCreated(Windows::UI::Core::CoreWindow^ window)
{
    if (m_WaitForDebuggerAttach)
    {
        WaitForDebuggerAttach();
    }

    m_window = window;

    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    if (m_window != nullptr)
    {
        m_WindowBoundsWidth = m_window.Get()->Bounds.Width;
        m_WindowBoundsHeight = m_window.Get()->Bounds.Height;

    #if !defined(_XBOX_ONE)
        // Scale window by DPI
        DpiAdjustWindowBounds(m_WindowBoundsWidth, m_WindowBoundsHeight);
    #endif
    }
    else
    {
        m_WindowBoundsWidth = 1920;
        m_WindowBoundsHeight = 1080;
    }

    m_NeedsResize = true;
}
#else
void D3D12App::WindowCreated()
{
    if (m_WaitForDebuggerAttach)
    {
        WaitForDebuggerAttach();
    }

    m_WindowBoundsWidth = 1920;
    m_WindowBoundsHeight = 1080;

    m_NeedsResize = true;
}
#endif

HRESULT D3D12App::FrameworkInitialize()
{
    m_TitleRunning = true;

    ZeroMemory( &m_Init, sizeof(m_Init) );
    m_Init.Width = (UINT32)m_WindowBoundsWidth;
    m_Init.Height = (UINT32)m_WindowBoundsHeight;
    PreWindowInit( &m_Init );

#if DEVTEST_USE_HWND
    CreateWin32Window();
#endif

    m_InitParams.DriverType = D3D_DRIVER_TYPE_HARDWARE;
#if defined(_GAMING_XBOX)
    m_InitParams.FeatureLevel = D3D_FEATURE_LEVEL_12_0;
#else
    m_InitParams.FeatureLevel = D3D_FEATURE_LEVEL_11_0;
#endif
    m_InitParams.BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_InitParams.DepthBufferFormat = DXGI_FORMAT_UNKNOWN;
    m_InitParams.MsaaSamples = 1;
    m_InitParams.MsaaQuality = 0;
    m_InitParams.CreateUISprite = TRUE;
#if defined(_GAMING_XBOX)
    m_InitParams.CreateDeviceParameters.Version = D3D12_SDK_VERSION;
    m_InitParams.CreateDeviceParameters.GraphicsCommandQueueRingSizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
    m_InitParams.CreateDeviceParameters.GraphicsScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
    m_InitParams.CreateDeviceParameters.ComputeScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
#endif
    m_InitParams.Fullscreen = FALSE;
    m_InitParams.SwapChainBufferCount = 3;
    m_InitParams.MaxRecordingCommandLists = 1;
    m_InitParams.RenderTargetHeapSize = 16;
    m_InitParams.DepthStencilHeapSize = 16;
    m_InitParams.EngineCount = 4;

    m_InitParams.DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
    m_InitParams.DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // s0
    m_InitParams.DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
    m_InitParams.RTSlot[0].InitAsDescriptorTable(1, &m_InitParams.DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // t0
    m_InitParams.RTSlot[1].InitAsDescriptorTable(1, &m_InitParams.DescRange[1], D3D12_SHADER_VISIBILITY_PIXEL); // s0
    m_InitParams.RTSlot[2].InitAsDescriptorTable(1, &m_InitParams.DescRange[2], D3D12_SHADER_VISIBILITY_ALL); // b0
    m_InitParams.RootSignatureSlotCount = 3;
    ZeroMemory(&m_InitParams.StaticSamplers, sizeof(m_InitParams.StaticSamplers));
    m_InitParams.StaticSamplerCount = 0;

#if defined(_GAMING_XBOX)
    m_InitParams.SwapChainFlags = 0;
#else
    m_InitParams.SwapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
#endif  

#if defined(_GAMING_XBOX)
    m_InitParams.UploadHeapSizeBytes = 8 * 1024 * 1024;
#else
    m_InitParams.UploadHeapSizeBytes = 32 * 1024 * 1024;
#endif

    m_hFenceWaitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    HRESULT hr = PreD3DInitialize( &m_InitParams );
    DX::ThrowIfFailed(hr);

#if defined(_GAMING_XBOX)
    DX::ThrowIfFailed( m_SuspendSemaphore.Initialize( m_InitParams.EngineCount ) );
#endif

#if DEVTEST_USE_GDK_INPUT
    hr = GameInputCreate(&m_pGameInput);
    DX::ThrowIfFailed(hr);
#endif

    hr = InitializeD3D();
    DX::ThrowIfFailed(hr);

    if (m_NeedsResize)
    {
        m_NeedsResize = false;
        CreateSwapChainAndTargets();
    }

    return hr;
}

void D3D12App::SuspendRendering()
{
#if defined(_GAMING_XBOX)
    DebugSpew( "App suspending rendering!\n" );
    m_SuspendSemaphore.Suspend();
    HRESULT hr = m_pd3dCmdQueue->SuspendX(0);
    DebugSpew( "App suspended rendering!\n" );
    assert( SUCCEEDED(hr) );
    m_RenderingPaused = true;
#endif
}

void D3D12App::ResumeRendering()
{
#if defined(_GAMING_XBOX)
    DebugSpew( "App resuming rendering!\n" );
    HRESULT hr = m_pd3dCmdQueue->ResumeX();
    assert( SUCCEEDED(hr) );

#if defined(XGS_BUILD)
    // Restore the frame interval:
    DX::ThrowIfFailed(m_pd3dDevice->SetFrameIntervalX(nullptr, D3D12XBOX_FRAME_INTERVAL_60_HZ, 1, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

    // Restore the frame start event:
    DX::ThrowIfFailed(m_pd3dDevice->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
#endif

    m_SuspendSemaphore.Resume();
    DebugSpew( "App resumed rendering!\n" );
    m_RenderingPaused = false;
#endif
}

#if (defined(_XBOX_ONE) && defined(_TITLE)) && !defined(XGS_BUILD)
inline void ApplyDeadZone(FLOAT& InputOutput)
#else
inline void ApplyDeadZone(DOUBLE& InputOutput)
#endif
{
    static const FLOAT s_DeadZone = 0.24f;
    FLOAT Value = (FLOAT)InputOutput;
    if (Value < -s_DeadZone)
    {
        Value += s_DeadZone;
    }
    else if (Value > s_DeadZone)
    {
        Value -= s_DeadZone;
    }
    else
    {
        InputOutput = 0.0;
        return;
    }
    Value = Value / (1.0 - s_DeadZone);
    Value = std::max(-1.0f, std::min(1.0f, Value));
    InputOutput = Value;
}

bool D3D12App::Tick()
{
#ifdef SCREEN_GRAB_INCLUDED
    // Do screenshots
    bool capturing = false;
    if (m_frameCountdown == m_FrameIndex + 1)
    {
        wchar_t tempWstr[1024];
        size_t sz = 0;

        if (m_pixCapture)
        {
            mbstowcs_s(&sz, tempWstr, _countof(tempWstr) - 1, m_pAppName, _TRUNCATE);
            tempWstr[sz] = 0;
            std::wstring name = (std::wstring(tempWstr) + L".pix3");

            capturing = SUCCEEDED(m_pd3dCmdQueue->PIXGpuBeginCapture(0, name.c_str()));
        }
    }
#endif

    m_FrameCmdAllocatorPoolIndex = (m_FrameCmdAllocatorPoolIndex + 1) % m_FrameCmdAllocatorPoolCount;
    m_pFrameCmdAllocator = m_pFrameCmdAllocatorPool[m_FrameCmdAllocatorPoolIndex];

    // Ensure that the fence associated with this command allocator has completed before the
    // command allocator is reset.
    const UINT64 FrameCompletedFence = m_FrameCmdAllocatorFenceValues[m_FrameCmdAllocatorPoolIndex];
    if (FrameCompletedFence > 0)
    {
        if (m_pGpuFence->GetCompletedValue() < FrameCompletedFence)
        {
            m_pGpuFence->SetEventOnCompletion(FrameCompletedFence, m_hFenceWaitEvent);
            WaitForSingleObjectEx(m_hFenceWaitEvent, INFINITE, FALSE);
        }
    }

    m_pFrameCmdAllocator->Reset();

    FRAME_PIPELINE_TOKEN FrameToken = Throttle();

    m_Timer.MarkFrame();

    bool RenderThisFrame = true;
    bool IsGamepadPresent = false;

    RawGamepadReading reading = {};
    bool SuppressKeyboardInput = false;

#if DEVTEST_USE_GDK_INPUT
    IGameInputReading* pGIR = nullptr;
    m_pGameInput->GetCurrentReading(GameInputKindGamepad, nullptr, &pGIR);
    IsGamepadPresent = (pGIR != nullptr);
#elif DEVTEST_USE_XINPUT
    XINPUT_STATE XGamepad = {};
    DWORD XInputResult = XInputGetState(0, &XGamepad);
    IsGamepadPresent = (XInputResult == ERROR_SUCCESS);
#else
    IVectorView<Gamepad^>^ gamepads = Gamepad::Gamepads;
    IsGamepadPresent = (gamepads->Size > 0);
#endif

    if (IsGamepadPresent)
    {
    #if DEVTEST_USE_GDK_INPUT
        GameInputGamepadState state;
        pGIR->GetGamepadState(&state);
        SAFE_RELEASE(pGIR);
        reading.LeftThumbstickX = state.leftThumbstickX;
        reading.LeftThumbstickY = state.leftThumbstickY;
        reading.RightThumbstickX = state.rightThumbstickX;
        reading.RightThumbstickY = state.rightThumbstickY;
        reading.LeftTrigger = state.leftTrigger;
        reading.RightTrigger = state.rightTrigger;
        reading.Buttons = (GamepadButtons)state.buttons;
    #elif DEVTEST_USE_XINPUT
        TranslateGamepad(&reading, &XGamepad);
    #else
        reading = gamepads->GetAt(0)->GetCurrentReading();
    #endif

        ApplyDeadZone(reading.LeftThumbstickX);
        ApplyDeadZone(reading.LeftThumbstickY);
        ApplyDeadZone(reading.RightThumbstickX);
        ApplyDeadZone(reading.RightThumbstickY);

        if (reading.LeftThumbstickX != 0 || reading.LeftThumbstickY != 0 ||
            reading.RightThumbstickX != 0 || reading.RightThumbstickY != 0)
        {
            SuppressKeyboardInput = true;
        }
    }

    if (!SuppressKeyboardInput)
    {
        if (m_EmulateLeftStickFromKeyboard)
        {
            FLOAT TargetLeftX = 0;
            FLOAT TargetLeftY = 0;
            if (m_TranslatedStickButtons & 0x1) TargetLeftX -= 1;
            if (m_TranslatedStickButtons & 0x2) TargetLeftX += 1;
            if (m_TranslatedStickButtons & 0x4) TargetLeftY -= 1;
            if (m_TranslatedStickButtons & 0x8) TargetLeftY += 1;
            const FLOAT InputLerpSpeed = 5.0f;
            FLOAT LerpValue = std::min(1.0f, (FLOAT)m_Timer.GetDeltaTime() * InputLerpSpeed);
            FLOAT NextLeftX = LerpValue * TargetLeftX + (1.0f - LerpValue) * (FLOAT)m_LastReading.LeftThumbstickX;
            FLOAT NextLeftY = LerpValue * TargetLeftY + (1.0f - LerpValue) * (FLOAT)m_LastReading.LeftThumbstickY;
            reading.LeftThumbstickX += NextLeftX;
            reading.LeftThumbstickY += NextLeftY;
        }
        else
        {
            if (m_TranslatedStickButtons & 0x1)
            {
                m_TranslatedKeyButtons |= (UINT32)GamepadButtons::A;
            }
        }

        const LONG DeltaX = InterlockedExchange(&m_MouseDeltaX, 0);
        const LONG DeltaY = InterlockedExchange(&m_MouseDeltaY, 0);
        const FLOAT TargetRightX = std::min(1.0f, std::max(-1.0f, (FLOAT)DeltaX / 3.0f));
        const FLOAT TargetRightY = std::min(1.0f, std::max(-1.0f, (FLOAT)DeltaY / 3.0f));
        reading.RightThumbstickX += TargetRightX;
        reading.RightThumbstickY += m_InvertMouseY ? TargetRightY : -TargetRightY;
    }

    UINT32 LastButtons = (UINT32)m_LastReading.Buttons;
    reading.Buttons = (GamepadButtons)((UINT32)reading.Buttons | m_TranslatedKeyButtons);
    const UINT32 CurrentButtons = (UINT32)reading.Buttons;
    m_PressedButtons = (LastButtons ^ CurrentButtons) & CurrentButtons;
    m_LastReading = reading;

    // Allow the game to exit by pressing the Back button on the Controller
    // This is just a helper for development.
    if (IsButtonPressed(GamepadButtons::View))
    {
        CloseApp();
        RenderThisFrame = false;
        m_RenderingPaused = true;
    }

    FLOAT LX = (FLOAT)reading.LeftThumbstickX;
    FLOAT LY = (FLOAT)reading.LeftThumbstickY;
    FLOAT AbsLX = fabsf(LX);
    FLOAT AbsLY = fabsf(LY);
    m_ThumbLeftX = (AbsLX < (7849.0f / 32768.0f)) ? 0.0f : LX;
    m_ThumbLeftY = (AbsLY < (7849.0f / 32768.0f)) ? 0.0f : LY;

    FLOAT RX = (FLOAT)reading.RightThumbstickX;
    FLOAT RY = (FLOAT)reading.RightThumbstickY;
    FLOAT AbsRX = fabsf(RX);
    FLOAT AbsRY = fabsf(RY);
    m_ThumbRightX = (AbsRX < (8689.0f / 32768.0f)) ? 0.0f : RX;
    m_ThumbRightY = (AbsRY < (8689.0f / 32768.0f)) ? 0.0f : RY;

    if (m_RenderingPaused &&
        (m_FrameIndex != 0))
    {
#if DBG
        DebugSpew( "D3D12App: Rendering suspended!\n" );
        Sleep( 100 );
#endif
        RenderThisFrame = false;
    }

    if (RenderThisFrame)
    {
        bool NeedsResize = m_NeedsResize;
        if (NeedsResize)
        {
            m_NeedsResize = false;
            m_Init.Width = (UINT32)m_WindowBoundsWidth;
            m_Init.Height = (UINT32)m_WindowBoundsHeight;
            CreateSwapChainAndTargets();
        }

        Update(NeedsResize || (m_FrameIndex == 0));
        Render(FrameToken);

        // Record the fence value associated with the current frame command allocator,
        // so we can track the completion of this frame before resetting the frame
        // command allocator in the future:
        m_FrameCmdAllocatorFenceValues[m_FrameCmdAllocatorPoolIndex] = m_CpuFence;

        // Advance main fence
        m_pd3dCmdQueue->Signal(m_pGpuFence, m_CpuFence++);

        m_DeferredReleaseQueue.ReleaseNow(m_pGpuFence);
    }

    ++m_FrameIndex;


#ifdef SCREEN_GRAB_INCLUDED
    if (capturing)
    {
        m_pd3dCmdQueue->PIXGpuEndCapture();
    }


    // Do screenshots
    if (m_frameCountdown == m_FrameIndex)
    {
        wchar_t tempWstr[1024];
        size_t sz = 0;

        if (m_screenshot)
        {
            mbstowcs_s(&sz, tempWstr, _countof(tempWstr) - 1, m_pAppName, _TRUNCATE);
            tempWstr[sz] = 0;
            std::wstring name = (std::wstring(tempWstr) + L"_screenshot.dds");

            DirectX::SaveDDSTextureToFile(
                m_pd3dDevice,
                m_pd3dCmdQueue,
                GetCurrentBackBufferTexture(),
                name.c_str());
        }

        // Exit
        return false;
    }
#endif


    return !m_ShutdownRequested;
}

bool D3D12App::IsButtonDown( GamepadButtons Button ) const
{
    return ((UINT32)m_LastReading.Buttons & (UINT32)Button) == (UINT32)Button;
}

void D3D12App::WindowClosing()
{
    HRESULT hr = Terminate();
    assert( SUCCEEDED(hr) );

    TerminateD3D();

#if DEVTEST_USE_GDK_INPUT
    SAFE_RELEASE(m_pGameInput);
#endif
}

HRESULT D3D12App::CreateSwapChainAndTargets()
{
    static bool s_DisableReentrancy = FALSE;
    if (s_DisableReentrancy)
    {
        return S_OK;
    }

    if (m_pd3dDevice == nullptr)
    {
        return E_FAIL;
    }

    BlockUntilIdle();
    for (UINT32 j = 0; j < 2; ++j)
    {
        for (UINT32 i = 0; i < ARRAYSIZE(m_pRenderTargetTexture[j]); ++i)
        {
            SAFE_RELEASE(m_pRenderTargetTexture[j][i]);
        }
    }
    SAFE_RELEASE(m_pDepthStencilTexture);

    UINT32 Width = m_Init.Width;
    UINT32 Height = m_Init.Height;

    if (m_InitParams.DriverType == D3D_DRIVER_TYPE_SOFTWARE)
    {
        Width = std::max(1U, Width / 4);
        Height = std::max(1U, Height / 4);
    }

    m_InitParams.MsaaSamples = std::max(1U, m_InitParams.MsaaSamples);

    HRESULT hr = S_OK;

#if !defined(_GAMING_XBOX)
    IDXGIFactory4* pIDXGIFactory = nullptr;
    DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&pIDXGIFactory)));
#endif

    m_InitParams.SwapChainBufferCount = std::min(m_InitParams.SwapChainBufferCount, (UINT)ARRAYSIZE(m_pRenderTargetTexture[0]));

#if defined(_GAMING_XBOX)
    D3D12_RESOURCE_DESC SwapTexDesc = {};
    SwapTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    SwapTexDesc.Alignment = 0;
    SwapTexDesc.Width = Width;
    SwapTexDesc.Height = Height;
    SwapTexDesc.DepthOrArraySize = 1;
    SwapTexDesc.Format = m_InitParams.BackBufferFormat;
    SwapTexDesc.MipLevels = 1;
    SwapTexDesc.SampleDesc.Count = 1;
    SwapTexDesc.SampleDesc.Quality = 0;
    SwapTexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    SwapTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (m_InitParams.HDR != FALSE)
    {
        SwapTexDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP;
    }

    for (UINT32 j = 0; j < (m_InitParams.Multiplane != FALSE ? 2 : 1); ++j)
    {
        for (UINT32 i = 0; i < m_InitParams.SwapChainBufferCount; ++i)
        {
            hr = CreateDefaultResource(m_pd3dDevice,
                &SwapTexDesc,
                D3D12_RESOURCE_STATE_PRESENT,
                (void**)&m_pRenderTargetTexture[j][i],
                DXGI_FORMAT_UNKNOWN,
                nullptr,
                true);
            if (FAILED(hr))
            {
                return hr;
            }
        }
    }
#else
    ZeroMemory(&m_SwapChainDesc, sizeof(m_SwapChainDesc));
    m_SwapChainDesc.BufferCount = m_InitParams.SwapChainBufferCount;
    m_SwapChainDesc.Width = Width;
    m_SwapChainDesc.Height = Height;
    m_SwapChainDesc.Format = m_InitParams.BackBufferFormat;
    m_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    m_SwapChainDesc.SampleDesc.Count = 1;
    m_SwapChainDesc.SampleDesc.Quality = 0;
    m_SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    m_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    m_SwapChainDesc.Flags = m_InitParams.SwapChainFlags;

    if (m_pSwapChain != nullptr)
    {
        hr = m_pSwapChain->ResizeBuffers(m_SwapChainDesc.BufferCount, m_SwapChainDesc.Width, m_SwapChainDesc.Height, m_SwapChainDesc.Format, m_SwapChainDesc.Flags);
    }
    else
    {
//        hr = pIDXGIFactory->CreateSwapChainForCoreWindow(m_pd3dCmdQueue, reinterpret_cast<IUnknown*>(CoreWindow::GetForCurrentThread()), &m_SwapChainDesc, nullptr, &m_pSwapChain);

        // Create a descriptor for the swap chain.
        m_SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//        m_SwapChainDesc.Flags = DXGIX_SWAP_CHAIN_MATCH_XBOX360_AND_PC;

    #if defined(_GAMING_DESKTOP) || PC_BUILD
        // Create a swap chain for the window.
        hr = pIDXGIFactory->CreateSwapChainForHwnd(
            m_pd3dCmdQueue,
            s_hWnd,
            &m_SwapChainDesc,
            nullptr,
            nullptr,
            &m_pSwapChain
            );
    #else
        #if PC_BUILD
            IUnknown* pWindow = nullptr;
        #else
            IUnknown* pWindow = reinterpret_cast<IUnknown*>(CoreWindow::GetForCurrentThread());
        #endif

        // Create a swap chain for the window.
        hr = pIDXGIFactory->CreateSwapChainForCoreWindow(
            m_pd3dCmdQueue,
            pWindow,// m_window,
            &m_SwapChainDesc,
            nullptr,
            &m_pSwapChain
            );
    #endif
    }
#endif

    if( FAILED(hr) )
    {
        return hr;
    }

#if !defined(_GAMING_XBOX)
    SAFE_RELEASE( pIDXGIFactory );
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    if( m_InitParams.Fullscreen )
    {
        s_DisableReentrancy = TRUE;

        hr = m_pSwapChain->SetFullscreenState( TRUE, nullptr );

        s_DisableReentrancy = FALSE;

        if( FAILED(hr) )
        {
            return hr;
        }
    }
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    for (UINT32 j = 0; j < (m_InitParams.Multiplane != FALSE ? 2 : 1); ++j)
#endif
    {
        for (UINT32 i = 0; i < m_InitParams.SwapChainBufferCount; ++i)
        {
            ID3D12Resource* pBackBuffer = NULL;
#if defined(_XBOX_ONE) && defined(_TITLE)
            pBackBuffer = m_pRenderTargetTexture[j][i];
            m_RenderTargetView[j][i] = m_RTHeap.hCPU(j * ARRAYSIZE(m_RenderTargetView[0]) + i);
            m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, m_RenderTargetView[j][i]);
#else
            hr = m_pSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (LPVOID*)&pBackBuffer);
            if (FAILED(hr))
            {
                return hr;
            }
            m_pRenderTargetTexture[0][i] = pBackBuffer;
            m_RenderTargetView[0][i] = m_RTHeap.hCPU(i);
            m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, m_RenderTargetView[0][i]);
#endif
        }
    }

    m_Viewport.Width = (FLOAT)Width;
    m_Viewport.Height = (FLOAT)Height;
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    m_ScissorRect.right = Width;
    m_ScissorRect.bottom = Height;

    m_AspectRatio = m_Viewport.Width / m_Viewport.Height;

    if( m_InitParams.DepthBufferFormat != DXGI_FORMAT_UNKNOWN )
    {
        D3D12_RESOURCE_DESC TexDesc = {};
        TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        TexDesc.Alignment = 0;
        TexDesc.Width = Width;
        TexDesc.Height = Height;
        TexDesc.DepthOrArraySize = 1;
        TexDesc.Format = m_InitParams.DepthBufferFormat;
        TexDesc.MipLevels = 1;
        TexDesc.SampleDesc.Count = m_InitParams.MsaaSamples;
        TexDesc.SampleDesc.Quality = m_InitParams.MsaaQuality;
        TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        TexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        hr = CreateDefaultResource(m_pd3dDevice, 
                                   &TexDesc, 
                                   D3D12_RESOURCE_STATE_DEPTH_WRITE, 
                                   (void**)&m_pDepthStencilTexture);
        if( FAILED(hr) )
        {
            return hr;
        }

        m_DepthStencilView = m_DSHeap.hCPU(0);
        m_pd3dDevice->CreateDepthStencilView( m_pDepthStencilTexture, NULL, m_DepthStencilView );
    }

    UINT32 BackBufferIndex = GetCurrentBackBufferIndex();
    if (BackBufferIndex != 0)
    {
        m_FrameIndex += (m_InitParams.SwapChainBufferCount - BackBufferIndex);
    }

    return S_OK;
}

HRESULT D3D12App::InitializeD3D()
{
    HRESULT hr;

#if defined(_GAMING_XBOX)

    hr = D3D12XboxCreateDevice(
        nullptr, 
        &m_InitParams.CreateDeviceParameters, 
        __uuidof(ID3D12Device), 
        (void**)&m_pd3dDevice);

#else

    if (m_InitParams.CreateDebugDevice)
    {
        ID3D12Debug* pDebug = nullptr;
        hr = D3D12GetDebugInterface(__uuidof(*pDebug), (void**)&pDebug);
        if (pDebug != nullptr)
        {
            pDebug->EnableDebugLayer();
        }
        SAFE_RELEASE(pDebug);
    }

    IUnknown* pSelectedAdapter = SelectAdapter();

    hr = D3D12CreateDevice(
        pSelectedAdapter,
        m_InitParams.FeatureLevel,
        __uuidof(ID3D12Device),
        (void**)&m_pd3dDevice );

    SAFE_RELEASE(pSelectedAdapter);

#endif

    if (FAILED(hr) || m_pd3dDevice == nullptr)
    {
        return hr;
    }

#if defined(_GAMING_XBOX)
    m_pd3dDevice->GetGpuHardwareConfigurationX(&m_HardwareConfig);

    if (IsScorpio())
    {
        wcscpy_s(m_SelectedAdapterDesc.Description, L"Xbox One X Game OS");
    }
    else
    {
        wcscpy_s(m_SelectedAdapterDesc.Description, L"Xbox One (S) Game OS");
    }
#endif


    m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_FeatureOptions, sizeof(m_FeatureOptions));

#if defined(_GAMING_XBOX)
    // Establish the frame interval:

    hr = m_pd3dDevice->SetFrameIntervalX(nullptr, D3D12XBOX_FRAME_INTERVAL_60_HZ, 1, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE);
    if (FAILED(hr))
    {
        return hr;
    }

    // Schedule the frame start event:

    hr = m_pd3dDevice->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE);
    if (FAILED(hr))
    {
        return hr;
    }
#endif

    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.NodeMask = D3D12XBOX_NODE_MASK;
    QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_pd3dDevice->CreateCommandQueue(&QueueDesc, __uuidof(*m_pd3dCmdQueue), (void**)&m_pd3dCmdQueue);

    if (m_pd3dCmdQueue == nullptr)
    {
        return E_FAIL;
    }

    m_pd3dCmdQueue->GetTimestampFrequency(&m_GpuTimestampFrequency);

    hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(*m_pCmdAllocator), (void**)&m_pCmdAllocator);
    if (FAILED(hr) || m_pCmdAllocator == nullptr)
    {
        return hr;
    }

    m_FrameCmdAllocatorPoolCount = m_InitParams.SwapChainBufferCount + 1;
    ASSERT(m_FrameCmdAllocatorPoolCount <= _countof(m_pFrameCmdAllocatorPool));

    for (UINT i = 0; i < m_FrameCmdAllocatorPoolCount; ++i)
    {
        hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(*m_pCmdAllocator), (void**)&m_pFrameCmdAllocatorPool[i]);
        if (FAILED(hr) || m_pFrameCmdAllocatorPool[i] == nullptr)
        {
            return hr;
        }
        m_FrameCmdAllocatorFenceValues[i] = 0;
    }
    m_FrameCmdAllocatorPoolIndex = 0;
    m_pFrameCmdAllocator = m_pFrameCmdAllocatorPool[m_FrameCmdAllocatorPoolIndex];

    hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(*m_pGpuFence), (void**)&m_pGpuFence);
    if (FAILED(hr) || m_pGpuFence == nullptr)
    {
        return hr;
    }
    m_CpuFence = 1;

    if (m_InitParams.RenderTargetHeapSize > 0)
    {
        hr = m_RTHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_InitParams.RenderTargetHeapSize);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    if (m_InitParams.DepthStencilHeapSize > 0)
    {
        hr = m_DSHeap.Initialize(m_pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_InitParams.DepthStencilHeapSize);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (m_InitParams.RootSignatureSlotCount > 0)
    {
        assert(m_InitParams.RootSignatureSlotCount <= ARRAYSIZE(m_InitParams.RTSlot));
        CD3DX12_ROOT_SIGNATURE_DESC RTLayout(m_InitParams.RootSignatureSlotCount, m_InitParams.RTSlot, m_InitParams.StaticSamplerCount, m_InitParams.StaticSamplers);
        RTLayout.Flags =  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* pSerializedLayout = nullptr;
        ID3DBlob* pErrorBlob = nullptr;
        hr = D3D12SerializeRootSignature(&RTLayout, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedLayout, &pErrorBlob);
        if (FAILED(hr))
        {
            DebugSpew("Root signature serialization error: %s\n", pErrorBlob->GetBufferPointer());
            SAFE_RELEASE(pErrorBlob);
            return hr;
        }

        hr = m_pd3dDevice->CreateRootSignature(
            D3D12XBOX_NODE_MASK,
            pSerializedLayout->GetBufferPointer(),
            pSerializedLayout->GetBufferSize(),
            __uuidof(ID3D12RootSignature),
            (void**)&m_pDefaultRootSignature);

        SAFE_RELEASE(pSerializedLayout);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (m_InitParams.UploadHeapSizeBytes > 0)
    {
        hr = m_UploadHeap.Initialize(m_pd3dDevice, m_pGpuFence, &m_CpuFence, m_InitParams.UploadHeapSizeBytes, false);
        if( FAILED(hr) )
        {
            return hr;
        }
    }

    if (m_InitParams.CreateUISprite)
    {
        assert(m_InitParams.UploadHeapSizeBytes >= 128 * 1024);

        SpriteInitParams12 IP = {};
        IP.pd3dDevice12 = m_pd3dDevice;
        IP.pd3dCmdQueue = m_pd3dCmdQueue;
        IP.pd3dCmdAllocatorDirect = m_pCmdAllocator;
        IP.pGpuFence = m_pGpuFence;
        IP.pCpuFence = &m_CpuFence;
        IP.pUploadHeap = &m_UploadHeap;
        hr = m_Sprite.Initialize( &IP );
        if( FAILED(hr) )
        {
            return hr;
        }
    }

    return S_OK;
}

VOID D3D12App::TerminateD3D()
{
    UINT64 FinalFence = InterlockedIncrement64((LONG64*)&m_CpuFence);
    if (m_pd3dCmdQueue != nullptr)
    {
        m_pd3dCmdQueue->Signal(m_pGpuFence, FinalFence);

        while (FinalFence > m_pGpuFence->GetCompletedValue())
        {
            Sleep(50);
        }
    }

    if( m_InitParams.CreateUISprite )
    {
        m_Sprite.Terminate();
    }

#if defined(_GAMING_XBOX)
    if (m_pd3dCmdQueue)
    {
        D3D12XBOX_PRESENT_PARAMETERS presentParameters = {0};
        presentParameters.Flags = D3D12XBOX_PRESENT_FLAG_IMMEDIATE;
        m_pd3dCmdQueue->PresentX(0, nullptr, &presentParameters);
    }
#endif

    SAFE_RELEASE( m_pDepthStencilTexture );
#if defined(_XBOX_ONE) && defined(_TITLE)
    for (UINT32 j = 0; j < (m_InitParams.Multiplane != FALSE ? 2 : 1); ++j)
#else
    UINT32 j = 0;
#endif
    {
        for (UINT32 i = 0; i < ARRAYSIZE(m_pRenderTargetTexture[j]); ++i)
        {
            SAFE_RELEASE(m_pRenderTargetTexture[j][i]);
        }
    }
#if !defined(_XBOX_ONE) || !defined(_TITLE)
    SAFE_RELEASE( m_pSwapChain );
#endif

    m_RTHeap.Terminate();
    m_DSHeap.Terminate();

    m_UploadHeap.Terminate();

    SAFE_RELEASE(m_pDefaultRootSignature);
    for (UINT i = 0; i < m_FrameCmdAllocatorPoolCount; ++i)
    {
        SAFE_RELEASE(m_pFrameCmdAllocatorPool[i]);
    }
    SAFE_RELEASE(m_pCmdAllocator);
    SAFE_RELEASE(m_pGpuFence);
    SAFE_RELEASE(m_pd3dCmdQueue);

#ifdef _DEBUG
//     ID3D11Debug* pDebug = nullptr;
//     m_pd3dDevice->QueryInterface(__uuidof(pDebug), (VOID**)&pDebug);
//     pDebug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
//     SAFE_RELEASE(pDebug);
#endif

    SAFE_RELEASE(m_pd3dDevice);
}

ID3D12GraphicsCommandList* D3D12App::CreateCommandList(bool FrameScopedUsage, ID3D12PipelineState* pPSO)
{
    ID3D12GraphicsCommandList* pd3dCmdList = nullptr;
    HRESULT hr = m_pd3dDevice->CreateCommandList(D3D12XBOX_NODE_MASK, 
                                                 D3D12_COMMAND_LIST_TYPE_DIRECT, 
                                                 FrameScopedUsage ? m_pFrameCmdAllocator : m_pCmdAllocator, 
                                                 pPSO, 
                                                 __uuidof(ID3D12GraphicsCommandList), 
                                                 (void**)&pd3dCmdList);
    if (SUCCEEDED(hr))
    {
        return pd3dCmdList;
    }
    return nullptr;
}

ID3D12GraphicsCommandList* D3D12App::CreateBundle()
{
    ID3D12GraphicsCommandList* pd3dCmdList = nullptr;
    HRESULT hr = m_pd3dDevice->CreateCommandList(D3D12XBOX_NODE_MASK, 
                                                 D3D12_COMMAND_LIST_TYPE_BUNDLE, 
                                                 m_pCmdAllocator, 
                                                 nullptr, 
                                                 __uuidof(ID3D12GraphicsCommandList), 
                                                 (void**)&pd3dCmdList);
    if (SUCCEEDED(hr))
    {
        return pd3dCmdList;
    }
    return nullptr;
}


#if !defined(_GAMING_XBOX)
IUnknown* D3D12App::SelectAdapter()
{
    IDXGIAdapter1* pSelectedAdapter = nullptr;
    IDXGIFactory2* pIDXGIFactory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(*pIDXGIFactory), (void**)&pIDXGIFactory);
    UINT32 AdapterIndex = 0;
    do
    {
        IDXGIAdapter1* pA = nullptr;
        hr = pIDXGIFactory->EnumAdapters1(AdapterIndex, &pA);
        if (pA != nullptr)
        {
            DXGI_ADAPTER_DESC1 ADesc = {};
            hr = pA->GetDesc1(&ADesc);
            DebugSpew("DXGI adapter %u: vendor %x name %S\n", AdapterIndex, ADesc.VendorId, ADesc.Description);
            if (ADesc.VendorId == 0x10de || ADesc.VendorId == 0x1002 || ADesc.VendorId == 0)
            {
                pSelectedAdapter = pA;
                m_SelectedAdapterDesc = ADesc;
                break;
            }
            else
            {
                SAFE_RELEASE(pA);
            }
        }
        ++AdapterIndex;
    } while (SUCCEEDED(hr));
    SAFE_RELEASE(pIDXGIFactory);
    return pSelectedAdapter;
}
#endif

FRAME_PIPELINE_TOKEN D3D12App::Throttle()
{
#if defined(_GAMING_XBOX)
    D3D12XBOX_FRAME_PIPELINE_TOKEN FrameToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;

    // Wait for the frame start event to be signaled:
    HRESULT hr = m_pd3dDevice->WaitFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN, 
        D3D12XBOX_WAIT_FRAME_EVENT_INFINITE,
        nullptr,
        D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE,
        &FrameToken);

    DX::ThrowIfFailed(hr);

    return FrameToken;
#else
    return 0;
#endif
}

void D3D12App::PrepareBackBufferForRendering(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBufferTexture)
{
    ResourceBarrier(pCmdList, pBackBufferTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void D3D12App::PrepareBackBufferForPresent(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pBackBufferTexture)
{
    ResourceBarrier(pCmdList, pBackBufferTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

HRESULT D3D12App::PerformPixGpuCapture(const WCHAR* strFileName)
{
    if (strFileName == nullptr)
    {
        strFileName = L"d:\\devtest12.pix3";
    }

#if defined(_GAMING_XBOX)
    return m_pd3dCmdQueue->PIXGpuCaptureNextFrame(0, strFileName);
#else
    return E_NOTIMPL;
#endif
}

void D3D12App::SetFullscreenMode(bool Fullscreen)
{
#if UWP_BUILD && !defined(_XBOX_ONE)
    bool IsFullscreenNow = IsFullscreenMode();
    auto applicationView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
    if (IsFullscreenNow && !Fullscreen)
    {
        applicationView->ExitFullScreenMode();
    }
    else if (!IsFullscreenNow && Fullscreen)
    {
        applicationView->TryEnterFullScreenMode();
    }
#endif
}

bool D3D12App::IsFullscreenMode()
{
#if !UWP_BUILD || defined(_XBOX_ONE)
    return true;
#else
    auto applicationView = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();
    return applicationView->IsFullScreenMode;
#endif
}