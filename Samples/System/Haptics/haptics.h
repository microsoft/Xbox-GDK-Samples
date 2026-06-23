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
    void Initialize(HWND hWnd);
    void Update();
    void Draw();
    void Shutdown();

private:
};

std::wstring OpenWavFileDialog(HWND owner);
bool IsButtonPressed(GameInputGamepadButtons buttons, GameInputGamepadButtons lastButtons, GameInputGamepadButtons button);

const char* StringifyDeviceId(_In_ const APP_LOCAL_DEVICE_ID& deviceId) noexcept;
