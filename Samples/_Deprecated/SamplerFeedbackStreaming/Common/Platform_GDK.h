//--------------------------------------------------------------------------------------
// Platform_GDK.h
//
// Include file for various platform headers used for the Game Development Kit.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifndef XGS_BUILD
// Xbox Graphics test code used a compilation define of XGS_BUILD=1 for GDK code before _GAMING_XBOX existed.
#define XGS_BUILD 1
#endif

#if !defined(_GAMING_XBOX)

#if defined(_XBOX_ONE)
#define _GAMING_XBOX
#elif !defined(_GAMING_DESKTOP)
#define _GAMING_DESKTOP
#endif

#endif

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#include <windows.h>

#ifdef _GAMING_XBOX
#define D3D12XBOX_NO_COMPAT_TYPEDEFS 1
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#else
#include <d3d12.h>
#include <dxgi1_4.h>
#include "uwp\d3dx12.h"
#endif

#include <pix.h>

#include <stdio.h>

#include <xGameRuntime.h>

#include <appnotify.h>

#include <exception>
#include <memory>
#include <stdexcept>

#ifdef _GAMING_XBOX
#include <GameInput.h>
#include <xmem.h>
#define DEVTEST_USE_GDK_INPUT 1
#else
#include <xinput.h>
#define DEVTEST_USE_XINPUT 1
#endif

#define DEVTEST_USE_HWND 1

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw com_exception(hr);
        }
    }
}

enum class GamepadButtons : UINT32
{
    None = 0x0,
    Menu = 0x1,
    View = 0x2,
    A = 0x4,
    B = 0x8,
    X = 0x10,
    Y = 0x20,
    DPadUp = 0x40,
    DPadDown = 0x80,
    DPadLeft = 0x100,
    DPadRight = 0x200,
    LeftShoulder = 0x400,
    RightShoulder = 0x800,
    LeftThumbstick = 0x1000,
    RightThumbstick = 0x2000,
    Paddle1 = 0x4000,
    Paddle2 = 0x8000,
    Paddle3 = 0x10000,
    Paddle4 = 0x20000,
};

struct RawGamepadReading
{
    UINT64 Timestamp;
    GamepadButtons Buttons;
    DOUBLE LeftTrigger;
    DOUBLE RightTrigger;
    DOUBLE LeftThumbstickX;
    DOUBLE LeftThumbstickY;
    DOUBLE RightThumbstickX;
    DOUBLE RightThumbstickY;
};
