///--------------------------------------------------------------------------------------
// main.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <algorithm>
#include <filesystem>

#include <ShellScalingApi.h>
#pragma comment(lib, "shcore.lib")

#include "atg/imgui_allocator.h"
#include "haptics.h"

static ImGuiStyle g_imGuiStyle{};
static float      g_uiScale = 1.0f;
static UINT       g_dpiX = 0, g_dpiY = 0;
constexpr float   DefaultDpi = 96.0f;

HWND g_hWnd;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SetUIScale(float scale);
HRESULT GetDeviceDpi(unsigned int* x, unsigned int* y);

int WINAPI wWinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Advanced Haptics", NULL };
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"Advanced Haptics", WS_OVERLAPPEDWINDOW, 100, 100, 1920, 1080, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hWnd, 1920, 1080))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = nullptr;
    ImFontConfig ifc{};
    ifc.SizePixels = 16;
    io.Fonts->AddFontDefault(&ifc);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_Sample_DX12_Init();

    // get current/default style for later use with DPI and resolution changes
    g_imGuiStyle = ImGui::GetStyle();
    GetDeviceDpi(&g_dpiX, &g_dpiY);
    g_uiScale = (g_dpiX / DefaultDpi);
    SetUIScale(g_uiScale);

    Sample_Initialize();

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        Sample_Update();

        // Start the Dear ImGui frame
        ImGui_Sample_DX12_PreRender();
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        Sample_Draw(g_uiScale);

        // Rendering
        ImGui::Render();

        ImGui_Sample_DX12_PostRender();
    }

    Sample_Shutdown(); 

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;

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

HRESULT GetDeviceDpi(unsigned int* x, unsigned int* y)
{
    return GetDpiForMonitor(MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY), MDT_EFFECTIVE_DPI, x, y);
}

void SetUIScale(float scale)
{
    // get current style and save off copy to restore colors later
    ImGuiStyle& currentStyle = ImGui::GetStyle();
    ImGuiStyle originalStyle = currentStyle;

    // clear out the style and scale a default style + font
    currentStyle = ImGuiStyle();
    currentStyle.ScaleAllSizes(scale);
    currentStyle.FontScaleDpi = scale;

    // replace the colors we saved off
    memcpy(currentStyle.Colors, originalStyle.Colors, sizeof(currentStyle.Colors));
}
