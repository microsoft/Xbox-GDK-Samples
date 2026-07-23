//--------------------------------------------------------------------------------------
// haptics.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

#define RETURN_IF_FAILED(hr) if(FAILED(hr)) return hr;
#define RETURN_IF_NULL_ALLOC(ptr) if(ptr == nullptr) return E_OUTOFMEMORY;

class Sample
{
public:
    Sample() = default;
    ~Sample() = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void Initialize(HWND hWnd);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_window = nullptr;
};

std::wstring OpenWavFileDialog(HWND owner);
bool IsButtonPressed(GameInputGamepadButtons buttons, GameInputGamepadButtons lastButtons, GameInputGamepadButtons button);

const char* StringifyDeviceId(_In_ const APP_LOCAL_DEVICE_ID& deviceId) noexcept;
