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

// In order for the peer network to operate, each peer needs to be able
// to deterministically generate a peer's network id based on their XUID.
// This isn't guaranteed to be unique, but is sufficient for the sample.
#define XUID_TO_PEER(x) (((x) & 0xFFFF0000) >> 16)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <grdk.h>

#if _GRDK_VER < 0x47BB2070 /* GDK Edition 191102 */
#error This sample requires the November 2020 GDK QFE2 or later
#endif

#include <d3d12.h>
#include <dxgi1_6.h>

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>

#include "d3dx12.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include <mutex>
#include <list>
#include <queue>
#include <string_view>

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>
#include "xal\xal.h"
#include "xsapi-c\services_c.h"

#include <XUser.h>
#include <XTaskQueue.h>
#include <XGame.h>
#include <XSystem.h>

#include "GamePad.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "RenderTargetState.h"
#include "DirectXHelpers.h"

#include <XGameRuntime.h>
#include <XGameUI.h>
#include <XUser.h>
#include <XAsync.h>

#include "GameChat2.h"

#include "StringUtil.h"
#include "Json.h"

#pragma comment(lib, "ws2_32.lib")

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
#pragma warning(default : 4061 4062 4191 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
