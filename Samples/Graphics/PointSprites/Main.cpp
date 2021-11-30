//--------------------------------------------------------------------------------------
// Main.cpp
//
// Entry point for Microsoft GDK with Xbox extensions
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PointSprites.h"

#include "ATGTelemetry.h"

#include <appnotify.h>
#include <XGameRuntimeInit.h>

using namespace DirectX;

namespace
{
    std::unique_ptr<Sample> g_sample;
    HANDLE g_plmSuspendComplete = nullptr;
    HANDLE g_plmSignalResume = nullptr;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!XMVerifyCPUSupport())
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: This hardware does not support the required instruction set.\n");
#ifdef __AVX2__
        OutputDebugStringA("This may indicate a Gaming.Xbox.Scarlett.x64 binary is being run on an Xbox One.\n");
#endif
#endif
        return 1;
    }

    if (FAILED(XGameRuntimeInitialize()))
        return 1;

    // Default main thread to CPU 0
    SetThreadAffinityMask(GetCurrentThread(), 0x1);

    // Microsoft GDKX supports UTF-8 everywhere
    assert(GetACP() == CP_UTF8);

    g_sample = std::make_unique<Sample>();

    // Register class and create window
    PAPPSTATE_REGISTRATION hPLM = {};
    {
        // Register class
        WNDCLASSEXA wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXA);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.lpszClassName = u8"PointSpritesWindowClass";
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        if (!RegisterClassExA(&wcex))
            return 1;

        // Create window
        HWND hwnd = CreateWindowExA(0, u8"PointSpritesWindowClass", u8"PointSprites", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080, nullptr, nullptr, hInstance,
            nullptr);
        if (!hwnd)
            return 1;

        ShowWindow(hwnd, nCmdShow);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_sample.get()));

        // Sample Usage Telemetry
        //
        // Disable or remove this code block to opt-out of sample usage telemetry
        ATG::SendLaunchTelemetry();

        g_sample->Initialize(hwnd);

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
                    (void)WaitForSingleObject(g_plmSuspendComplete, INFINITE);
                }
                else
                {
                    SetEvent(g_plmSignalResume);
                }
            }, hwnd, &hPLM))
            return 1;
    }

    // Main message loop
    MSG msg = {};
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

    UnregisterAppStateChangeNotification(hPLM);

    CloseHandle(g_plmSuspendComplete);
    CloseHandle(g_plmSignalResume);

    XGameRuntimeUninitialize();

    return static_cast<int>(msg.wParam);
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto sample = reinterpret_cast<Sample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_USER:
        if (sample)
        {
            sample->OnSuspending();

            // Complete deferral
            SetEvent(g_plmSuspendComplete);

            (void)WaitForSingleObject(g_plmSignalResume, INFINITE);

            sample->OnResuming();
        }
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
void ExitSample() noexcept
{
    PostQuitMessage(0);
}
