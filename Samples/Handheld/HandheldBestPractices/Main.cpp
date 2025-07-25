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

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI wWinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR lpCmdLine, _In_ int /*nCmdShow*/)
{
    bool windowed = false;

    int numArgs = 0;
    auto argv = CommandLineToArgvW(lpCmdLine, &numArgs);
    for(int i = 0; i < numArgs; i++)
    {
        if(wcscmp(argv[i], L"-windowed") == 0)
        {
            windowed = true;
        }
    }

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Handheld Best Practices", nullptr };
    RegisterClassExW(&wc);

    HWND hWnd   = CreateWindowW(wc.lpszClassName, L"Handheld Best Practices", windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, windowed ? 1920 : CW_USEDEFAULT, windowed ? 1080 : CW_USEDEFAULT, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hWnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window, top of z-order
    ShowWindow(hWnd, windowed ? SW_SHOWNORMAL : SW_SHOWMAXIMIZED);
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
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_Sample_DX12_Init();

    Sample_Initialize(hWnd);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        Sample_Update();

        if(ImGui_Sample_DX12_PreRender())
            continue;

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Sample_Draw();

        // Rendering
        ImGui::Render();

        ImGui_Sample_DX12_PostRender();
    }

    WaitForLastSubmittedFrame();

    Sample_Shutdown();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // insert our sample's WndProc before passing messages off to ImGui or Windows
    if (Sample_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_SIZE:
            ImGui_Sample_DX12_Resize(lParam, wParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
