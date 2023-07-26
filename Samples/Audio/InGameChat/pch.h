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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <grdk.h>

#if _GRDK_VER < 0x55F00C58 /* GDK Edition 220300 */
#error This sample requires the March 2022 GDK or later
#endif

#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#elif defined(_GAMING_XBOX)
#include <d3d12_x.h>
#include <d3dx12_x.h>
#else
#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "d3dx12.h"
#endif

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <WinSock2.h>
#include <ws2tcpip.h>

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
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#ifdef _GAMING_XBOX
#include <pix3.h>
#else
// To use graphics markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>
#endif
#include "xal\xal.h"
#include "xsapi-c\services_c.h"

#include <XGameRuntime.h>
#include <XGameUI.h>
#include <XUser.h>
#include <XTaskQueue.h>
#include <XGame.h>
#include <XGameInvite.h>
#include <XSystem.h>

#ifdef _GAMING_XBOX
#include <GameInput.h>
#endif

#include "Party.h"
#include "PartyXboxLive.h"
#include "GameChat2.h"

#include "StringUtil.h"
#include "Json.h"

#include "DirectXHelpers.h"
#include "GamePad.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "RenderTargetState.h"

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

constexpr XUserLocalId NULL_USER_LOCAL_ID = {};

inline bool operator==(const XUserLocalId& lhs, const XUserLocalId& rhs) noexcept
{
    return lhs.value == rhs.value;
}

inline bool operator!=(const XUserLocalId& lhs, const XUserLocalId& rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator<(const XUserLocalId& lhs, const XUserLocalId& rhs) noexcept
{
    return lhs.value < rhs.value;
}

inline bool operator>(const XUserLocalId& lhs, const XUserLocalId& rhs) noexcept
{
    return lhs.value > rhs.value;
}

inline bool operator==(const APP_LOCAL_DEVICE_ID& lhs, const APP_LOCAL_DEVICE_ID& rhs) noexcept
{
    return std::equal(std::cbegin(lhs.value), std::cend(lhs.value), std::cbegin(rhs.value), std::cend(rhs.value));
}

inline bool operator!=(const APP_LOCAL_DEVICE_ID& lhs, const APP_LOCAL_DEVICE_ID& rhs) noexcept
{
    return !(lhs == rhs);
}

inline bool operator<(const APP_LOCAL_DEVICE_ID& lhs, const APP_LOCAL_DEVICE_ID& rhs) noexcept
{
    return std::lexicographical_compare(std::cbegin(lhs.value), std::cend(lhs.value), std::cbegin(rhs.value), std::cend(rhs.value), std::less<uint8_t>());
}

inline bool operator>(const APP_LOCAL_DEVICE_ID& lhs, const APP_LOCAL_DEVICE_ID& rhs) noexcept
{
    return std::lexicographical_compare(std::cbegin(lhs.value), std::cend(lhs.value), std::cbegin(rhs.value), std::cend(rhs.value), std::greater<uint8_t>());
}

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4242 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
