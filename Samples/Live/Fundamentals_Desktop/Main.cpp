//--------------------------------------------------------------------------------------
// Main.cpp
//
// Entry point for Microsoft Game Development Kit (GDK)
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Fundamentals_Desktop.h"

#include <XGameRuntimeInit.h>
#include <XGameErr.h>

#include "ATGTelemetry.h"

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    std::unique_ptr<Sample> g_sample;
}

LPCWSTR g_szAppName = L"PC Fundamentals Sample";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitSample() noexcept;

int SampleMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!XMVerifyCPUSupport())
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: This hardware does not support the required instruction set.\n");
#endif
        return 1;
    }

    // Initialize COM for WIC usage
#ifdef USING_WINDOWS_GAMING_INPUT
    // The following Windows Runtime initialization is only needed for scenarios where DirectX Tool Kit's GamePad is
    // using Windows.Gaming.Input instead of GameInput or XINPUT. GRTS does not require it.
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        return 1;
#else
    if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED)))
        return 1;
#endif

    // Initialize the GameRuntime
    HRESULT hr = XGameRuntimeInitialize();
    if (FAILED(hr))
    {
        if (hr == E_GAMERUNTIME_DLL_NOT_FOUND || hr == E_GAMERUNTIME_VERSION_MISMATCH)
        {
            std::ignore = MessageBoxW(nullptr, L"Game Runtime is not installed on this system or needs updating.", g_szAppName, MB_ICONERROR | MB_OK);
        }
        return 1;
    }

    g_sample = std::make_unique<Sample>();

    // Register class and create window
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"Fundamentals_DesktopWindowClass";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
            return 1;

        // Create window
        int w, h;
        g_sample->GetDefaultSize(w, h);

        RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(0, L"Fundamentals_DesktopWindowClass", g_szAppName, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance,
            g_sample.get());

        if (!hwnd)
            return 1;

        ShowWindow(hwnd, nCmdShow);
        
        // Sample Usage Telemetry
        //
        // Disable or remove this code block to opt-out of sample usage telemetry
        ATG::SendLaunchTelemetry();

        GetClientRect(hwnd, &rc);

        g_sample->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);
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

    XGameRuntimeUninitialize();

#ifndef USING_WINDOWS_GAMING_INPUT
    CoUninitialize();
#endif

    return static_cast<int>(msg.wParam);
}

// Entry point
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
    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;
    // Set s_fullscreen to true if defaulting to fullscreen.

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

    case WM_PAINT:
        if (s_in_sizemove && sample)
        {
            sample->Tick();
        }
        else
        {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DISPLAYCHANGE:
        if (sample)
        {
            sample->OnDisplayChange();
        }
        break;

    case WM_MOVE:
        if (sample)
        {
            sample->OnWindowMoved();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!s_minimized)
            {
                s_minimized = true;
                if (!s_in_suspend && sample)
                    sample->OnSuspending();
                s_in_suspend = true;
            }
        }
        else if (s_minimized)
        {
            s_minimized = false;
            if (s_in_suspend && sample)
                sample->OnResuming();
            s_in_suspend = false;
        }
        else if (!s_in_sizemove && sample)
        {
            sample->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_ENTERSIZEMOVE:
        s_in_sizemove = true;
        break;

    case WM_EXITSIZEMOVE:
        s_in_sizemove = false;
        if (sample)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            sample->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
        }
        break;

    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
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

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && sample)
                sample->OnSuspending();
            s_in_suspend = true;
            return TRUE;

        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && sample)
                    sample->OnResuming();
                s_in_suspend = false;
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_INPUT:
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
    case WM_MOUSEHOVER:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        {
            // Implements the classic ALT+ENTER fullscreen toggle
            if (s_fullscreen)
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

                int width = 800;
                int height = 600;
                if (sample)
                    sample->GetDefaultSize(width, height);

                ShowWindow(hWnd, SW_SHOWNORMAL);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            else
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                ShowWindow(hWnd, SW_SHOWMAXIMIZED);
            }

            s_fullscreen = !s_fullscreen;
        }
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_MOUSEACTIVATE:
        // When you click activate the window, we want Mouse to ignore that event.
        return MA_ACTIVATEANDEAT;

    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitSample() noexcept
{
    PostQuitMessage(0);
}
