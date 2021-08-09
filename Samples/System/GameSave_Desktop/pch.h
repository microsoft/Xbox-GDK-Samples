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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <time.h>

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#include "xal\xal.h"
#include "xsapi-c\services_c.h"

#include <XUser.h>
#include <XTaskQueue.h>
#include <XGame.h>
#include <XGameSave.h>
#include <XSystem.h>

#include "GamePad.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "RenderTargetState.h"

#include "Audio.h"
#include "CommonStates.h"
#include "DescriptorHeap.h"
#include "ResourceUploadBatch.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

// Project helpers
#include "Common\DPIHelpers.h"
#include "GlobalConstants.h"
#include "Common\Log.h"
#include "Common\StringHelpers.h"

#define Stringize(arg)                                   #arg
#define Stringize2(arg)                                  Stringize( arg )
#define COMPILER_OUTPUT_MSG(strMsg)                      message( __FILE__ "(" Stringize2(__LINE__) "): " strMsg )

// Show controls for PC version
#define RENDER_ARBITRARY_USER_CONTROLS 1

namespace DX
{
    struct RectangleF
    {
        union {
            float Left;
            float X;
        };
        union {
            float Top;
            float Y;
        };
        float Width;
        float Height;

        RectangleF()
        {

        }

        RectangleF(float left, float top, float width, float height) :
            Left(left), Top(top), Width(width), Height(height)
        {
        }
    };

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

    inline void ThrowLastError()
    {
        throw com_exception(HRESULT_FROM_WIN32(GetLastError()));
    }

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

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4242 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
