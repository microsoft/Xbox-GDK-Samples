///--------------------------------------------------------------------------------------
// haptics.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

extern std::unique_ptr<AppLog> g_appLog;
extern std::unique_ptr<DX::DeviceResources> g_deviceResources;

#define LOG(s, ...) \
    g_appLog->AddLog(s, __VA_ARGS__);

#define LOG_IF_FAILED(f) \
{ \
    HRESULT hr = f; \
    if(FAILED(hr)) { g_appLog->AddLog("%08X - "#f"\n", hr); } \
}

#define LOG_AND_RETURN_IF_FAILED(f) \
{ \
    HRESULT hr = f; \
    g_appLog->AddLog("%08X - "#f"\n", hr); \
    if(FAILED(hr)) return; \
}

#define LOG_IF_FAILED_AND_RETURN(f) \
{ \
    HRESULT hr = f; \
    if(FAILED(hr)) { g_appLog->AddLog("%08X - "#f"\n", hr); return; } \
}

#define LOG_AND_CONTNUE(f) \
{ \
    HRESULT hr = f; \
    g_appLog->AddLog("%08X - "#f"\n", hr); \
}

#define RETURN_IF_FAILED(hr) if(FAILED(hr)) return hr;
#define RETURN_IF_NULL_ALLOC(ptr) if(ptr == nullptr) return E_OUTOFMEMORY;

void Sample_Initialize();
void Sample_Update();
void Sample_Draw(float uiScale);
void Sample_Shutdown();

std::wstring OpenWavFileDialog(HWND owner);
bool IsButtonPressed(GameInputGamepadButtons buttons, GameInputGamepadButtons lastButtons, GameInputGamepadButtons button);

const char* StringifyDeviceId(_In_ const APP_LOCAL_DEVICE_ID& deviceId) noexcept;

typedef struct StreamingFeedbackProvider
{
    IMMDevice* endpoint = nullptr;
    uint32_t locationCount = 0;
    GUID locations[GAMEINPUT_HAPTIC_MAX_LOCATIONS] = {};

    ~StreamingFeedbackProvider()
    {
        if (endpoint != nullptr)
        {
            endpoint->Release();
        }
    }
} StreamingFeedbackProvider;
