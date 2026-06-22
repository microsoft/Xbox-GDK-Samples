//--------------------------------------------------------------------------------------
// Platform_Xbox.h
//
// Include file for various platform headers used for the Xbox title OS (ERA) platform.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <wrl.h>
#include <xdk.h>
#include <d3d12_x.h>
#include <d3dx12_x.h>
#include <agile.h>

#include <pix.h>

#include <stdio.h>

using namespace Windows::Xbox::Input;

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
