//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#define _ENABLE_EXTENDED_ALIGNED_STORAGE

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
#include <WinSock2.h>

#include <Windows.h>

#include <wrl.h>
#include <wrl/client.h>

#include <grdk.h>

#if _GRDK_VER < 0x55F0110A /* GRDK Edition 220600 */
#error This sample requires the March 2022 GDK or later
#endif

#if defined(_M_ARM64)
#if _GRDK_EDITION < 260400
#error ARM64 support requires April 2026 GDK or later
#endif
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

#ifdef _GAMING_XBOX
#include <pix3.h>
#else
// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>
#endif

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <exception>
#include <functional>
#include <future>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>

#include "StepTimer.h"
#include "Texture.h"

#include "Audio.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "GamePad.h"
#include "Keyboard.h"
#include "SimpleMath.h"
#include "SpriteFont.h"

#include "Managers.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "Party.h"

// Suppress warnings from PlayFab Party Xbox Live header
#pragma warning(push)
#pragma warning(disable : 28285) // SAL syntax error in partyxboxlive.h
#include "PartyXboxLive.h"
#pragma warning(pop)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "Debug.h"

#include <XGameRuntime.h>

#include <xsapi-c/services_c.h>

#include "ArrayView.h"
#include "AsyncHelper.h"
#include "XboxHandle.h"
#include "GuidUtil.h"

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

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wswitch"
#endif

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4191 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
