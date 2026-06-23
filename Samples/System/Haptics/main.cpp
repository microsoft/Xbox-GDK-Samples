//--------------------------------------------------------------------------------------
// main.cpp
//
// Window setup and message loop
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "haptics.h"

namespace
{
    static constexpr int c_WindowWidth = 1920;
    static constexpr int c_WindowHeight = 1080;

    static std::unique_ptr<ImGuiAtg::DeviceContext> g_d3dDeviceContext;
    static std::unique_ptr<Sample> g_sample;
}

HWND g_hWnd;

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI wWinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Advanced Haptics", nullptr };
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowW(wc.lpszClassName, L"Advanced Haptics", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, c_WindowWidth, c_WindowHeight, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    g_d3dDeviceContext = std::make_unique<ImGuiAtg::DeviceContext>();
    if (!g_d3dDeviceContext->CreateDevice(g_hWnd, c_WindowWidth, c_WindowHeight))
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window, top of z-order
    ShowWindow(g_hWnd, SW_SHOWNORMAL);
    UpdateWindow(g_hWnd);
    SetForegroundWindow(g_hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = nullptr;                                 // Disable INI file creation

    // Setup Dear ImGui style
    ImGuiAtg::SetAtgStyle();
    ImGuiAtg::SetDpiScale(g_hWnd);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    g_d3dDeviceContext->DX12_Init();

    g_sample = std::make_unique<Sample>();
    g_sample->Initialize(g_hWnd);

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
    DestroyWindow(g_hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 1;
    }

    switch (msg)
    {
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
