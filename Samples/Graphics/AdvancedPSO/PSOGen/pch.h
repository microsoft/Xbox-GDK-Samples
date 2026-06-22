//-----------------------------------------------------------------------------
// pch.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <stdio.h>

#include <fstream>
#include <vector>
#include <set>
#include <string>

#include <wrl/client.h>

#ifdef _XBOXONE
#include <d3d12_x.h>
#include <d3dx12_x.h>
#else
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#endif

#include "WriteData.h"
#include "ReadData.h"

// To opt-out of telemetry uncomment the following line
//#define ATG_DISABLE_TELEMETRY

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        virtual const char* what() const noexcept override
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
}
