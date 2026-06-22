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

#include <grdk.h>

#if _GRDK_VER < 0x65F41800 /* GDK Edition 251000 */
#error This sample requires the October 2025 GDK or later
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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <iterator>
#include <locale>
#include <map>
#include <memory>
#include <random>
#include <stack>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>

#ifdef _GAMING_XBOX
#include <pix3.h>
#else
// To use graphics markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>
#endif

#include "DDSTextureLoader.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "GamePad.h"
#include "GraphicsMemory.h"
#include "ResourceUploadBatch.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "RenderTargetState.h"
#include "SimpleMath.h"

enum TestType : uint32_t
{
    Doing_Nothing,
    Doing_Async,
    Doing_Sync,
    Doing_DStorage_Async,
    Doing_DStorage_Sync,
    Doing_DStorage_Async_zlib,
    Doing_DStorage_Sync_zlib,
};

enum OverlapDepth : uint32_t
{
    FIRST_OVERLAP_DEPTH,
    depth_1 = FIRST_OVERLAP_DEPTH,
    depth_2,
    depth_4,
    depth_8,
    depth_12,
    depth_16,
    depth_24,
    depth_32,
    depth_64,
    depth_128,
    depth_256,
    depth_512,
    depth_768,
    depth_1024,
    depth_2048,
    depth_4096,
    LAST_OVERLAP_DEPTH
};

enum LoadOrder : uint32_t
{
    FIRST_LOAD_ORDER,
    TrueSequential = FIRST_LOAD_ORDER,
    Random,
    LAST_PERF_ORDER = Random,               // we really only care about the sequential and random cases, the others are special and must be explicitly chosen
    RandomSequential,
    Backwards,
    Redundant,
    LAST_LOAD_ORDER
};
enum DataSize : uint32_t
{
    FIRST_DATA_SIZE,
    size_8k = FIRST_DATA_SIZE,
    size_12k,
    size_16k,
    size_32k,
    size_64k,
    size_128k,
    size_192k,
    size_256k,
    size_512k,
    size_1024k,
    size_2048k,
    size_4096k,
    size_8192k,
    size_16384k,
    size_32768k,
    LAST_DATA_SIZE
};

constexpr uint32_t c_MinQDIDepth = FIRST_OVERLAP_DEPTH;
constexpr uint32_t c_MaxQDIDepth = LAST_OVERLAP_DEPTH;

OverlapDepth ConvertStringToOverlapDepth(const std::wstring& str);
std::wstring ConvertOverlapDepthToString(OverlapDepth overlapDepth);
uint32_t ConvertOverlapDepthToValue(OverlapDepth overlapDepth);
inline std::wstring ConvertOverlapDepthToString(uint32_t overlapDepth) { return ConvertOverlapDepthToString((OverlapDepth)overlapDepth); }
inline uint32_t ConvertOverlapDepthToValue(uint32_t overlapDepth) { return ConvertOverlapDepthToValue((OverlapDepth)overlapDepth); }

LoadOrder ConvertStringToLoadOrder(const std::wstring& str);
DataSize ConvertStringToDataSize(const std::wstring& str);

std::wstring ConvertLoadOrderToString(LoadOrder loadOrder);
inline std::wstring ConvertLoadOrderToString(uint32_t loadOrder) { return ConvertLoadOrderToString((LoadOrder)loadOrder); }

std::wstring ConvertDataSizeToString(DataSize dataSize);
uint32_t ConvertDataSizeToInteger(DataSize dataSize);
inline std::wstring ConvertDataSizeToString(uint32_t dataSize) { return ConvertDataSizeToString((DataSize)dataSize); }
inline uint32_t ConvertDataSizeToInteger(uint32_t dataSize) { return ConvertDataSizeToInteger((DataSize)dataSize); }

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
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)

//#pragma warning(disable : 4365)
