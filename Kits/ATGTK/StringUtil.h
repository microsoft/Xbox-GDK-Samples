//--------------------------------------------------------------------------------------
// StringUtil.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <string>

namespace DX
{
    size_t GetWideLength(const char* utf8String, size_t utf8Length);
    size_t GetUtf8Length(const wchar_t* wideString, size_t wideLength);

    std::wstring Utf8ToWide(const char* utf8String, size_t utf8Length);
    std::string WideToUtf8(const wchar_t* wideString, size_t wideLength);

    std::wstring Utf8ToWide(const std::string& utf8String);
    std::string WideToUtf8(const std::wstring& wideString);

    std::u32string Utf8ToUtf32(const std::string& utf8String);
    std::string Utf32ToUtf8(const std::u32string& utf32String);

    std::string ToLower(const std::string& utf8String);
    void ToLowerInPlace(std::string& utf8String);
    std::wstring ToLower(const std::wstring& wideString);
    void ToLowerInPlace(std::wstring& wideString);

    std::string ToUpper(const std::string& utf8String);
    void ToUpperInPlace(std::string& utf8String);
    std::wstring ToUpper(const std::wstring& wideString);
    void ToUpperInPlace(std::wstring& wideString);

    uint32_t DetermineUtf8CharBytesFromFirstByte(char byte);
    char32_t Utf8ToUtf32Character(const char* c, int charSize);
}
