//--------------------------------------------------------------------------------------
// File: NetworkDebugHelpers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#define NETWORK_LOGGING

#ifdef NETWORK_LOGGING
    #define DebugLog(format, ...) do { char charBuffer[1024]{}; sprintf_s(charBuffer, format "\n", __VA_ARGS__); OutputDebugStringA(charBuffer); } while(false)
    #define Assert(condition) do { if(!(condition)) { __debugbreak(); }} while(false)
#else
    #define DebugLog(...) (void)(__VA_ARGS__)
    #define Assert(...) (void)(__VA_ARGS__)
#endif

