//--------------------------------------------------------------------------------------
// Platform_UWP.h
//
// Include file for various platform headers used for the Universal Windows platform.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "uwp\d3dx12.h"
#include <agile.h>

#include <pix.h>

#include <stdio.h>

using namespace Windows::Gaming::Input;
typedef Windows::Gaming::Input::GamepadReading RawGamepadReading;

using namespace Windows::Foundation;

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            OutputDebugStringA("Fatal error launching devtest!!\n");

            // Set a breakpoint on this line to catch DirectX API errors
            throw Platform::Exception::CreateException(hr);
        }
    }
}

#define UWP_BUILD 1
