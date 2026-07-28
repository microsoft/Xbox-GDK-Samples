//--------------------------------------------------------------------------------------
// File: LogUtil.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LogUtil.h"

using namespace TextRenderer;

void LogUtil::Log(const char* text)
{
    OutputDebugStringA(text);
    OutputDebugStringA(u8"\n");
}
void LogUtil::Log(const char* text, const char* fontName, uint32_t fontSize)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"FontManager{%s, FontSize:%d} %s", fontName, fontSize, text);
    Log(buffer);
}
void LogUtil::LogFailedHR(const char* functionName, HRESULT hr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s failed with hr=%08X", functionName, hr);
    Log(buffer);
}
void LogUtil::LogFailedHR(const char* functionName, const char* fontName, HRESULT hr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Error for Font{%s} %s failed with hr=%08X", fontName, functionName, hr);
    Log(buffer);
}
void LogUtil::LogFailedHR(const char* functionName, const char* fontName, uint32_t fontSize, HRESULT hr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Error for Font{%s, FontSize:%d} %s failed with hr=%08X", fontName, fontSize, functionName, hr);
    Log(buffer);
}
void LogUtil::LogFailedFT(const char* functionName, const char* fontName, uint32_t fontSize, FT_Error ftErr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Error for Font{%s, FontSize:%d} %s failed with FT_Error=%08X", fontName, fontSize, functionName, ftErr);
    Log(buffer);
}
void LogUtil::LogFailedFT(const char* functionName, const char* fontName, FT_Error ftErr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Error for Font{%s} %s failed with FT_Error=%08X", fontName, functionName, ftErr);
    Log(buffer);
}
