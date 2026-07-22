//--------------------------------------------------------------------------------------
// main.cpp
//
// Window setup and message loop
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "HandheldBestPractices.h"

namespace
{
    static constexpr int c_WindowWidth = 1920;
    static constexpr int c_WindowHeight = 1080;

    static std::unique_ptr<ImGuiAtg::DeviceContext> g_d3dDeviceContext;
    static std::unique_ptr<Sample> g_sample;
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI wWinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"HandheldBestPractices", nullptr };
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowW(wc.lpszClassName, L"HandheldBestPractices", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, c_WindowWidth, c_WindowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    g_d3dDeviceContext = std::make_unique<ImGuiAtg::DeviceContext>();
    if (!g_d3dDeviceContext->CreateDevice(hWnd, c_WindowWidth, c_WindowHeight))
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window, top of z-order
    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = nullptr;                                 // Disable INI file creation

    // Setup Dear ImGui style
    ImGuiAtg::SetAtgStyle();
    ImGuiAtg::SetDpiScale(hWnd);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    g_d3dDeviceContext->DX12_Init();

    g_sample = std::make_unique<Sample>();
    g_sample->Initialize(hWnd);

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

    g_d3dDeviceContext.reset();
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // insert our sample's WndProc before passing messages off to ImGui or Windows
    if (g_sample && g_sample->WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 1;
    }

    // now give ImGui it's chance at the message
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 1;
    }

    // and now let us respond to any messages we care about that ImGui doesn't handle
    switch (msg)
    {
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

        case WM_DPICHANGED:
            {
                RECT* rect = (RECT*)lParam;
                SetWindowPos(hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER);
                ImGuiAtg::SetDpiScale(hWnd);
            }
            return 0;

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
