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

#include <XGameRuntime.h>

#include <memory>
#include <string>

