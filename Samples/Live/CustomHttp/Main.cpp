//--------------------------------------------------------------------------------------
// Main.cpp
//
// Entry point for Microsoft Game Development Kit (GDK)
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CustomHttp.h"

#include <appnotify.h>
#include <XDisplay.h>
#include <XGameRuntimeInit.h>
#include <XGameErr.h>

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    std::unique_ptr<Sample> g_sample;

#ifdef _GAMING_XBOX
    HANDLE g_plmSuspendComplete = nullptr;
    HANDLE g_plmSignalResume = nullptr;
#endif
}

#ifdef _GAMING_XBOX
bool g_HDRMode = false;
#endif

LPCWSTR g_szAppName = L"CustomHttp";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
#ifdef _GAMING_XBOX
void SetDisplayMode() noexcept;
#endif
void ExitSample() noexcept;

// Entry point
int WINAPI SampleMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!XMVerifyCPUSupport())
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: This hardware does not support the required instruction set.\n");
#if defined(_GAMING_XBOX) && defined(__AVX2__)
        OutputDebugStringA("This may indicate a Gaming.Xbox.Scarlett.x64 binary is being run on an Xbox One.\n");
#endif
#endif
        return 1;
    }

    // Initialize COM for WIC usage
    if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED)))
        return 1;

    // Initialize the GameRuntime
    HRESULT hr = XGameRuntimeInitialize();
    if (FAILED(hr))
    {
        if (hr == E_GAMERUNTIME_DLL_NOT_FOUND || hr == E_GAMERUNTIME_VERSION_MISMATCH)
        {
        }
        return 1;
    }

#ifdef _GAMING_XBOX
    // Default main thread to CPU 0
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
#endif

    g_sample = std::make_unique<Sample>();

    // Register class and create window
#ifdef _GAMING_XBOX
    PAPPSTATE_REGISTRATION hPLM = {};
    PAPPCONSTRAIN_REGISTRATION hPLM2 = {};
#endif
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"CustomHttpWindowClass";
        if (!RegisterClassExW(&wcex))
            return 1;

        // Create window
        RECT rc = { 0, 0, 1920, 1080 };

    HWND hwnd = CreateWindowExW(0, L"CustomHttpWindowClass", g_szAppName, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance,
            g_sample.get());
        if (!hwnd)
            return 1;

#ifdef _GAMING_XBOX
        SetDisplayMode();
#endif

        ShowWindow(hwnd, nCmdShow);

        GetClientRect(hwnd, &rc);

        g_sample->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);

#ifdef _GAMING_XBOX

        g_plmSuspendComplete = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        g_plmSignalResume = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (!g_plmSuspendComplete || !g_plmSignalResume)
            return 1;

        if (RegisterAppStateChangeNotification([](BOOLEAN quiesced, PVOID context)
        {
            if (quiesced)
            {
                ResetEvent(g_plmSuspendComplete);
                ResetEvent(g_plmSignalResume);

                // To ensure we use the main UI thread to process the notification, we self-post a message
                PostMessage(reinterpret_cast<HWND>(context), WM_USER, 0, 0);

                // To defer suspend, you must wait to exit this callback
                std::ignore = WaitForSingleObject(g_plmSuspendComplete, INFINITE);
            }
            else
            {
                SetEvent(g_plmSignalResume);
            }
        }, hwnd, &hPLM))
            return 1;

        if (RegisterAppConstrainedChangeNotification([](BOOLEAN constrained, PVOID context)
        {
            // To ensure we use the main UI thread to process the notification, we self-post a message
            SendMessage(reinterpret_cast<HWND>(context), WM_USER + 1, (constrained) ? 1u : 0u, 0);
        }, hwnd, &hPLM2))
            return 1;

#endif // _GAMING_XBOX
    }

    // Main message loop
    MSG msg = {};
    OutputDebugStringA("INFO: Sample started.\n");
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_sample->Tick();
        }
    }

    g_sample.reset();

#ifdef _GAMING_XBOX
    UnregisterAppStateChangeNotification(hPLM);
    UnregisterAppConstrainedChangeNotification(hPLM2);

    CloseHandle(g_plmSuspendComplete);
    CloseHandle(g_plmSignalResume);
#endif

    XGameRuntimeUninitialize();

    return static_cast<int>(msg.wParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    try
    {
        return SampleMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA("*** ERROR: Unhandled C++ exception thrown: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA(" *** \n");
        return 1;
    }
    catch (...)
    {
        OutputDebugStringA("*** ERROR: Unknown unhandled C++ exception thrown ***\n");
        return 1;
    }
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto sample = reinterpret_cast<Sample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        if (lParam)
        {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;

    case WM_ACTIVATEAPP:
        if (sample)
        {
            Keyboard::ProcessMessage(message, wParam, lParam);
            Mouse::ProcessMessage(message, wParam, lParam);

            if (wParam)
            {
                sample->OnActivated();
            }
            else
            {
                sample->OnDeactivated();
            }
        }
        break;

    case WM_ACTIVATE:
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

#ifdef _GAMING_XBOX

    case WM_USER:
        if (sample)
        {
            sample->OnSuspending();

            // Complete deferral
            SetEvent(g_plmSuspendComplete);

            std::ignore = WaitForSingleObject(g_plmSignalResume, INFINITE);

            SetDisplayMode();

            sample->OnResuming();
        }
        break;

    case WM_USER + 1:
        if (sample)
        {
            if (wParam)
            {
                sample->OnConstrained();
            }
            else
            {
                SetDisplayMode();

                sample->OnUnConstrained();
            }
        }
        break;

#endif // _GAMING_XBOX

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

#ifdef _GAMING_XBOX
// HDR helper
void SetDisplayMode() noexcept
{
    if (g_sample && g_sample->RequestHDRMode())
    {
        // Request HDR mode.
        auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, nullptr);

        g_HDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
        OutputDebugStringA((g_HDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif

        g_sample->OnDisplayChange();
    }
}
#endif

// Exit helper
void ExitSample() noexcept
{
    PostQuitMessage(0);
}
