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

// ImGui Desktop needs GDI
#ifdef _GAMING_XBOX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#endif // !_GAMING_XBOX

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <grdk.h>

#if _GRDK_VER < 0x65F41800 /* GDK Edition 251000 */
#error This sample requires the October 2025 GDK or later
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

#define _XM_NO_XMVECTOR_OVERLOADS_

#include <DirectXMath.h>
#include <DirectXColors.h>

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
#include <filesystem>
#include <functional>
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

// GameInput (the Microsoft.GameInput NuGet package overrides the GDK version on all platforms)
#include <GameInput.h>
#if GAMEINPUT_API_VERSION != 3
#error This sample requires the GameInput v3 API from Microsoft.GameInput 3.5 or later.
#endif
using namespace GameInput::v3;

#ifdef _GAMING_XBOX
#include <pix3.h>
#else
// To use graphics markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>
#endif

#include <XGame.h>
#include <XSystem.h>

// DirectXTK12 -- used by the 3D controller model viewer (ImGuiAtg::ModelViewer)
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DescriptorHeap.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GraphicsMemory.h"
#include "Model.h"
#include "RenderTargetState.h"
#include "ResourceUploadBatch.h"
#include "SimpleMath.h"

#include "StringUtil.h"

// Dear ImGui + ATG ImGui helpers
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include "imgui/imgui_atg.h"
#include "imgui/imgui_atg_render_target.h"
#include "imgui/imgui_atg_model_viewer.h"

// To opt-out of telemetry uncomment the following line
//#define ATG_DISABLE_TELEMETRY

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4263 4264 4265 4266 4289 4365 4746 4826 4841 4986 4987 5029 5038 5042)
