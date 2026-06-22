//--------------------------------------------------------------------------------------
// SampleWinHttpProxy.h
//
// Forward declarations for WinHTTP proxy APIs that may not be declared in older
// Windows SDK versions of winhttp.h but are present in winhttp.lib.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <winhttp.h>

extern "C" WINHTTPAPI DWORD WINAPI WinHttpCreateProxyResolver(HINTERNET hSession, HINTERNET* phResolver);
