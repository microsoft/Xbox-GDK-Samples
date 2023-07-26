//--------------------------------------------------------------------------------------
// pch.h
//
// Header for standard system include files.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

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

#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <gxdk.h>

#if _GXDK_VER < 0x55F00C58 /* GDK Edition 220300 */
#error This sample requires the March 2022 GDK or later
#endif

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#else
#include <d3d12_x.h>
#include <d3dx12_x.h>
#endif

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXHelpers.h>
#include <WICTextureLoader.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <exception>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include <pix3.h>

#include <XGame.h>
#include <XSystem.h>
#include <XUser.h>
#include <XGameErr.h>
#include <GameInput.h>
#include <XTaskQueue.h>

#include "CommonStates.h"
#include "DescriptorHeap.h"
#include "GraphicsMemory.h"
#include "RenderTargetState.h"
#include "ResourceUploadBatch.h"
#include "SpriteFont.h"
#include "SimpleMath.h"

#include "ATGColors.h"

#define UITK_ENABLE_FREETYPE
#include "UITK.h"

// To opt-out of telemetry uncomment the following line
//#define ATG_DISABLE_TELEMETRY

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const noexcept override
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
#ifdef _DEBUG
            char str[64] = {};
            sprintf_s(str, "**ERROR** Fatal Error with HRESULT of %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(str);
            __debugbreak();
#endif
            throw com_exception(hr);
        }
    }
}

// Comparison operators for APP_LOCAL_DEVICE_ID
inline bool operator==(const APP_LOCAL_DEVICE_ID& deviceId1, const APP_LOCAL_DEVICE_ID& deviceId2)
{
    return memcmp(deviceId1.value, deviceId2.value, APP_LOCAL_DEVICE_ID_SIZE) == 0;
}
inline bool operator!=(const APP_LOCAL_DEVICE_ID& deviceId1, const APP_LOCAL_DEVICE_ID& deviceId2)
{
    return !operator==(deviceId1, deviceId2);
}

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4242 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
