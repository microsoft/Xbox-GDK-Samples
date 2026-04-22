//--------------------------------------------------------------------------------------
// File: DebugLog.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <cassert>
#include <cstdarg>
#include <string_view>
#include <functional>

inline std::mutex& GetDebugLogMutex()
{
    static std::mutex s_mutex;
    return s_mutex;
}

// Optional callback invoked after OutputDebugStringA.
// Set this to forward log lines to an on-screen console.
inline std::function<void(const char*)>& GetDebugLogCallback()
{
    static std::function<void(const char*)> s_callback;
    return s_callback;
}

template <size_t bufferSize = 2048>
void ConsoleWriteLine(std::string_view format, ...)
{
    assert(format.size() < bufferSize && "format string is too large, split up the string or increase the buffer size");

    char buffer[bufferSize] = "";

    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format.data(), args);
    va_end(args);

    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");

    std::lock_guard<std::mutex> lock(GetDebugLogMutex());
    auto& cb = GetDebugLogCallback();
    if (cb)
    {
        cb(buffer);
    }
}
