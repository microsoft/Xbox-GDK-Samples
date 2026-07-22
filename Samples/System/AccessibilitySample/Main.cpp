//--------------------------------------------------------------------------------------
// Main.cpp
//
// Window setup and message loop
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AccessibilitySample.h"

#include <appnotify.h>
#include <XGameRuntimeInit.h>
#include <XGameErr.h>

#include "ATGTelemetry.h"

namespace
{
    static constexpr int c_WindowWidth = 1920;
    static constexpr int c_WindowHeight = 1080;

    static std::unique_ptr<ImGuiAtg::DeviceContext> g_d3dDeviceContext;
    static std::unique_ptr<Sample> g_sample;

#ifdef _GAMING_XBOX
    HANDLE g_plmSuspendComplete = nullptr;
    HANDLE g_plmSignalResume = nullptr;
#endif
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef _GAMING_XBOX
static void ToggleFullscreen(HWND hWnd)
{
    static bool isFullscreen = false;
    static RECT windowRect = {};

    if (isFullscreen)
    {
        // Restore windowed mode
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP,
            windowRect.left, windowRect.top,
            windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
            SWP_FRAMECHANGED);
    }
    else
    {
        // Switch to borderless fullscreen
        GetWindowRect(hWnd, &windowRect);
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
    }
    isFullscreen = !isFullscreen;
}
#endif

// Main code
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
#ifdef _GAMING_DESKTOP
    // Set working directory to exe location
    char dir[_MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, dir, _MAX_PATH) > 0)
    {
        std::string exe = dir;
        exe = exe.substr(0, exe.find_last_of("\\"));
        std::ignore = SetCurrentDirectoryA(exe.c_str());
    }
#endif

    HRESULT hr = XGameRuntimeInitialize();
    if (FAILED(hr))
    {
#ifdef _GAMING_DESKTOP
        if (hr == E_GAMERUNTIME_DLL_NOT_FOUND || hr == E_GAMERUNTIME_VERSION_MISMATCH)
        {
            std::ignore = MessageBoxW(nullptr, L"Game Runtime is not installed on this system or needs updating.", L"AccessibilitySample", MB_ICONERROR | MB_OK);
        }
#endif
        return 1;
    }

#ifdef _GAMING_XBOX
    SetThreadAffinityMask(GetCurrentThread(), 0x1);
#endif

    // Create application window
#ifndef _GAMING_XBOX
    ImGui_ImplWin32_EnableDpiAwareness();
#endif

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"AccessibilitySample", nullptr };
    RegisterClassExW(&wc);

#ifdef _GAMING_XBOX
    int width = 1920;
    int height = 1080;
#else
    int width = c_WindowWidth;
    int height = c_WindowHeight;
#endif

    HWND hWnd = CreateWindowW(wc.lpszClassName, L"AccessibilitySample", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    g_d3dDeviceContext = std::make_unique<ImGuiAtg::DeviceContext>();
    if (!g_d3dDeviceContext->CreateDevice(hWnd, width, height))
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hWnd, SW_SHOWNORMAL);
#ifndef _GAMING_XBOX
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
#endif

    // Sample Usage Telemetry
    ATG::SendLaunchTelemetry();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = nullptr;                                 // Disable INI file creation

    // Setup Dear ImGui style
    ImGuiAtg::SetAtgStyle();
#ifndef _GAMING_XBOX
    ImGuiAtg::SetDpiScale(hWnd);
#endif

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    g_d3dDeviceContext->DX12_Init();

    g_sample = std::make_unique<Sample>();
    g_sample->Initialize(hWnd);

#ifdef _GAMING_XBOX
    // Setup PLM suspend/resume
    PAPPSTATE_REGISTRATION hPLM = {};
    g_plmSuspendComplete = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    g_plmSignalResume = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

    RegisterAppStateChangeNotification([](BOOLEAN quiesced, PVOID context)
    {
        if (quiesced)
        {
            ResetEvent(g_plmSuspendComplete);
            ResetEvent(g_plmSignalResume);
            PostMessage(reinterpret_cast<HWND>(context), WM_USER, 0, 0);
            std::ignore = WaitForSingleObject(g_plmSuspendComplete, INFINITE);
        }
        else
        {
            SetEvent(g_plmSignalResume);
        }
    }, hWnd, &hPLM);
#endif

    // Main loop
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_sample->Update();

            // Start the Dear ImGui frame
            g_d3dDeviceContext->DX12_PreRender();
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGuiAtg::HandleStandardInput();

            // Draw the ImGui controls
            g_sample->Draw();

            // Rendering
            ImGui::Render();
            g_d3dDeviceContext->DX12_PostRender();
        }
    }

    // Shutdown and cleanup
    g_d3dDeviceContext->DX12_Shutdown();
    g_sample->Shutdown();
    g_sample.reset();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

#ifdef _GAMING_XBOX
    UnregisterAppStateChangeNotification(hPLM);
    if (g_plmSuspendComplete)
    {
        CloseHandle(g_plmSuspendComplete);
    }
    if (g_plmSignalResume)
    {
        CloseHandle(g_plmSignalResume);
    }
#endif

    XGameRuntimeUninitialize();

    g_d3dDeviceContext.reset();
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Give the sample first crack at messages before passing them to ImGui or the default proc.
    if (g_sample && g_sample->WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 1;
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 1;
    }

    switch (msg)
    {
#ifdef _GAMING_XBOX
        case WM_USER:
            if (g_sample)
            {
                g_sample->Suspend(g_d3dDeviceContext.get());
                SetEvent(g_plmSuspendComplete);
                std::ignore = WaitForSingleObject(g_plmSignalResume, INFINITE);
                g_sample->Resume(g_d3dDeviceContext.get());
            }
            break;
#endif

#ifndef _GAMING_XBOX
        case WM_GETMINMAXINFO:
            {
                // Enforce a minimum window size on desktop.
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                mmi->ptMinTrackSize.x = 1280;
                mmi->ptMinTrackSize.y = 720;
            }
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_F11)
            {
                ToggleFullscreen(hWnd);
                return 0;
            }
            break;

        case WM_SYSKEYDOWN:
            if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
            {
                ToggleFullscreen(hWnd);
                return 0;
            }
            break;
#endif

        case WM_ACTIVATEAPP:
            if (g_sample)
            {
                if (wParam)
                {
                    g_sample->Activated();
                }
                else
                {
                    g_sample->Deactivated();
                }
            }
            break;

        case WM_SIZE:
            if (g_d3dDeviceContext)
                g_d3dDeviceContext->DX12_Resize(lParam, wParam);
            return 0;

#ifndef _GAMING_XBOX
        case WM_DPICHANGED:
            {
                RECT* rect = (RECT*)lParam;
                SetWindowPos(hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER);
                ImGuiAtg::SetDpiScale(hWnd);
            }
            return 0;
#endif

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            {
                return 0;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void ExitSample() noexcept
{
    PostQuitMessage(0);
}
