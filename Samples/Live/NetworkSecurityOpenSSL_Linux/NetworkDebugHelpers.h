//--------------------------------------------------------------------------------------
// File: NetworkDebugHelpers.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

extern bool g_debugOutput;

#define DebugLog(format, ...)                       \
    if(g_debugOutput)                               \
    {                                               \
        char buffer[1024]{};                        \
        sprintf(buffer, format, ## __VA_ARGS__);    \
        std::cerr << buffer << std::endl;           \
    }
