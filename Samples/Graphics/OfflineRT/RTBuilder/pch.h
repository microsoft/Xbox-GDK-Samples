//-----------------------------------------------------------------------------
// pch.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

#include <wrl/client.h>

// CRT includes:
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

// GDK includes:
#include <gxdk.h>

#if _GXDK_VER < 0x55F00C58 /* GDK Edition 220301 */
#error This sample requires the March 2022 Update 1 GDK or later
#endif

// These end up being included from $(GXDKLatest)\toolKit\include\Scarlett. The
// GDK ships with special builds of the Scarlett and Xbox One graphics drivers
// that can be loaded on PC. The implementation of these driver builds reside
// inside the DLLs in $(GXDKLatest)\bin\Scarlett|XboxOne. This project has a
// custom build step that deploys the DLLs to the output directory so they can
// be resolved at runtime.
//
// The Scarlett and Xbox One drivers on PC expose functionality to create
// pipeline state objects, raytracing state object and acceleration
// structures. The drivers (along with the XG library for texture data) can be
// embedded directly into game asset pipelines to avoid runtime processing
// overhead for data on the Xbox One and Scarlett platforms.

#include <d3d12_xs.h>
#include <d3dx12_xs.h>

// ATGTK includes:
#include <ReadData.h>
#include <ReadCompressedData.h>
#include <WriteData.h>

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
            sprintf_s(s_str, "Failure with HRESULT of %08X", result);
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
            throw com_exception(hr);
        }
    }

    // Helper utility to throw on failed condition
    inline void ThrowIfFalse(bool cond, const char* message)
    {
        if (!cond)
        {
            throw std::exception(message);
        }
    }
}
