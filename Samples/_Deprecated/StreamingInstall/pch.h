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

#if _GXDK_VER < 0x4A610D2B /* GXDK Edition 200600 */
#error This sample requires the June 2020 GDK or later
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

#include <XPackage.h>
#include <XGame.h>
#include <XSystem.h>
#include <XTaskQueue.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <map>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <pix3.h>

#include "CommonStates.h"
#include "DescriptorHeap.h"
#include "Effects.h"
#include "GamePad.h"
#include "GraphicsMemory.h"
#include "PrimitiveBatch.h"
#include "RenderTargetState.h"
#include "SimpleMath.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "VertexTypes.h"

#include "TextConsole.h"

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

    // Helper to draw 2d quads in a similar way to previous sample
    inline void DrawQuadFromViewport(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>& primitiveBatch,
        D3D12_VIEWPORT viewport,
        DirectX::XMVECTOR color)
    {
        DirectX::VertexPositionColor vpc[] =
        {
            { DirectX::SimpleMath::Vector3(viewport.TopLeftX, viewport.TopLeftY, 0.f), color },
            { DirectX::SimpleMath::Vector3(viewport.TopLeftX + viewport.Width, viewport.TopLeftY, 0.f), color },
            { DirectX::SimpleMath::Vector3(viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height, 0.f), color },
            { DirectX::SimpleMath::Vector3(viewport.TopLeftX, viewport.TopLeftY + viewport.Height, 0.f), color },
        };

        primitiveBatch.DrawQuad(vpc[0], vpc[1], vpc[2], vpc[3]);
    }

    // Helper to draw text centered around origin
    inline void DrawCenteredText(DirectX::SpriteFont* font, DirectX::SpriteBatch* batch, const wchar_t* text,
        DirectX::XMFLOAT2 const& position, DirectX::FXMVECTOR color)
    {
        RECT bounds = font->MeasureDrawBounds(text, DirectX::XMFLOAT2(0,0));

        float centeredX = position.x - (bounds.right - bounds.left) / 2.f;
        float centeredY = position.y - (bounds.bottom - bounds.top) / 2.f;

        font->DrawString(batch, text, DirectX::XMFLOAT2(centeredX, centeredY), color);
    }
}

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
