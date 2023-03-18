//--------------------------------------------------------------------------------------
// File: LogUtil.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>

// FreeType2 is required for glyph rendering
#include <freetype/freetype.h>

namespace TextRenderer
{
    class LogUtil
    {
    public:
        static void Log(const char* text);
        static void Log(const char* text, const char* fontName, uint32_t fontSize);
        static void LogFailedHR(const char* functionName, HRESULT hr);
        static void LogFailedHR(const char* functionName, const char* fontName, uint32_t fontSize, HRESULT hr);
        static void LogFailedFT(const char* functionName, const char* fontName, uint32_t fontSize, FT_Error ftErr);
        static void LogFailedFT(const char* functionName, const char* fontName, FT_Error ftErr);
        static void LogFailedHR(const char* functionName, const char* fontName, HRESULT hr);
    };
}
