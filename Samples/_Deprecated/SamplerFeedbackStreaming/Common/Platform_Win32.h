//--------------------------------------------------------------------------------------
// Platform_Win32.h
//
// Include file for various platform headers used for the Win32 platform.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>

#include <dxgi1_4.h>
#include <d3d12.h>
#include "uwp\d3dx12.h"

#include <pix.h>

#include <exception>
#include <memory>
#include <stdexcept>

#include <stdio.h>

#include <xinput.h>
#define DEVTEST_USE_XINPUT 1

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
