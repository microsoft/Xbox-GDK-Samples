//--------------------------------------------------------------------------------------
// StringUtil.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StringUtil.h"
#include <cctype>
#include <string>

namespace
{
    constexpr DWORD MBConversionFlags = MB_ERR_INVALID_CHARS;
    constexpr DWORD WCConversionFlags = WC_ERR_INVALID_CHARS;

    // UTF8 <-> UTF32 bits
    // Mask removes bits from the byte to check.
    // Val is the value to compare against after removing bits via the mask.
    constexpr int ONE_BYTE_MASK = 0x80;   // b10000000
    constexpr int ONE_BYTE_VAL = 0x00;    // b0-------
    constexpr int ONE_BYTE_CONT = 0x7F;   // b01111111
    constexpr int TWO_BYTE_MASK = 0xE0;   // b11100000
    constexpr int TWO_BYTE_VAL = 0xC0;    // b110-----
    constexpr int TWO_BYTE_CONT = 0x1F;   // b00011111
    constexpr int THREE_BYTE_MASK = 0xF0; // b11110000
    constexpr int THREE_BYTE_VAL = 0xE0;  // b1110----
    constexpr int THREE_BYTE_CONT = 0x0F; // b00001111
    constexpr int FOUR_BYTE_MASK = 0xF8;  // b11111000
    constexpr int FOUR_BYTE_VAL = 0xF0;   // b11110---
    constexpr int FOUR_BYTE_CONT = 0x07;  // b00000111
    constexpr int MUL_BYTE_MASK = 0xC0;   // b11000000
    constexpr int MUL_BYTE_VAL = 0x80;    // b10------
    constexpr int MUL_BYTE_CONT = 0x3F;   // b00111111

    static char32_t ConvertUTF8ToUTF32_FourBytes(char firstByte, char secondByte, char thirdByte, char fourthByte)
    {
        // Four bytes are encoded such that the 3 least significant bits of the first byte and 6 least significant
        // bits of the following bytes are combined to create the full representation:
        // 00000000 00011122 22223333 33444444

        // Validate extra bytes
        if ((secondByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_FourBytes: Second byte does not start with proper 2 bits: %d", static_cast<uint32_t>(secondByte));
            throw std::exception(buf);
        }
        if ((thirdByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_FourBytes: Third byte does not start with proper 2 bits: %d", static_cast<uint32_t>(thirdByte));
            throw std::exception(buf);
        }
        if ((fourthByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_FourBytes: Fourth byte does not start with proper 2 bits: %d", static_cast<uint32_t>(fourthByte));
            throw std::exception(buf);
        }

        return
            static_cast<char32_t>(firstByte & FOUR_BYTE_CONT) << 18 |
            static_cast<char32_t>(secondByte & MUL_BYTE_CONT) << 12 |
            static_cast<char32_t>(thirdByte & MUL_BYTE_CONT) << 6 |
            static_cast<char32_t>(fourthByte & MUL_BYTE_CONT);
    }

    static char32_t ConvertUTF8ToUTF32_ThreeBytes(char firstByte, char secondByte, char thirdByte)
    {
        // Three bytes are encoded such that the 4 least significant bits of the first byte and 6 least significant
        // bits of the following bytes are combined to create the full representation:
        // 00000000 00000000 11112222 22333333

        // Validate extra bytes
        if ((secondByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_ThreeBytes: Second byte does not start with proper 2 bits: %d", static_cast<uint32_t>(secondByte));
            throw std::exception(buf);
        }
        if ((thirdByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_ThreeBytes: Third byte does not start with proper 2 bits: %d", static_cast<uint32_t>(thirdByte));
            throw std::exception(buf);
        }

        return
            static_cast<char32_t>(firstByte & THREE_BYTE_CONT) << 12 |
            static_cast<char32_t>(secondByte & MUL_BYTE_CONT) << 6 |
            static_cast<char32_t>(thirdByte & MUL_BYTE_CONT);
    }

    static char32_t ConvertUTF8ToUTF32_TwoBytes(char firstByte, char secondByte)
    {
        // Two bytes are encoded such that the 5 least significant bits of the first byte and the 6 least significant
        // bits of the second byte are combined to create the full representation:
        // 00000000 00000000 00000111 11222222

        // Validate extra bytes
        if ((secondByte & MUL_BYTE_MASK) != MUL_BYTE_VAL)
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32_TwoBytes: Second byte does not start with proper 2 bits: %d", static_cast<uint32_t>(secondByte));
            throw std::exception(buf);
        }

        return
            static_cast<char32_t>(firstByte & TWO_BYTE_CONT) << 6 |
            static_cast<char32_t>(secondByte & MUL_BYTE_CONT);
    }

    static char32_t ConvertUTF8ToUTF32_OneByte(char firstByte)
    {
        return static_cast<char32_t>(firstByte & ONE_BYTE_CONT);
    }
}

// Get the wchar length of a utf8 string
size_t DX::GetWideLength(const char* utf8String, size_t utf8Length)
{
    const int newLen = ::MultiByteToWideChar(
        CP_UTF8,
        MBConversionFlags,
        utf8String,
        static_cast<int>(utf8Length),
        nullptr,
        0
    );

    return static_cast<size_t>(newLen);
}

// Get the utf8 length of a wchar string
size_t DX::GetUtf8Length(const wchar_t* wideString, size_t wideLength)
{
    const int newLen = ::WideCharToMultiByte(
        CP_UTF8,
        WCConversionFlags,
        wideString,
        static_cast<int>(wideLength),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    return static_cast<size_t>(newLen);
}

std::wstring DX::Utf8ToWide(const char* utf8String, size_t utf8Length)
{
    std::wstring dest;
    const size_t wideLength = GetWideLength(utf8String, utf8Length);
    dest.resize(wideLength);
    ::MultiByteToWideChar(
        CP_UTF8,
        MBConversionFlags,
        utf8String,
        static_cast<int>(utf8Length),
        &dest[0],
        static_cast<int>(wideLength)
    );

    return dest;
}

std::string DX::WideToUtf8(const wchar_t* wideString, size_t wideLength)
{
    std::string dest;
    const size_t utf8Length = GetUtf8Length(wideString, wideLength);
    dest.resize(utf8Length);

    ::WideCharToMultiByte(
        CP_UTF8,
        WCConversionFlags,
        wideString,
        static_cast<int>(wideLength),
        &dest[0],
        static_cast<int>(utf8Length),
        nullptr,
        nullptr
    );

    return dest;
}

std::wstring DX::Utf8ToWide(const std::string& utf8String)
{
    return Utf8ToWide(utf8String.c_str(), utf8String.length());
}

std::string DX::WideToUtf8(const std::wstring& wideString)
{
    return WideToUtf8(wideString.c_str(), wideString.length());
}

std::u32string DX::Utf8ToUtf32(const std::string& utf8String)
{
    std::u32string outStr;
    size_t stringSize = utf8String.size();
    for (size_t index = 0; index < stringSize;)
    {
        char firstByte = utf8String[index];
        if ((firstByte & ONE_BYTE_MASK) == ONE_BYTE_VAL)
        {
            outStr.push_back(ConvertUTF8ToUTF32_OneByte(firstByte));
            ++index;
        }
        else if ((firstByte & TWO_BYTE_MASK) == TWO_BYTE_VAL)
        {
            if (index + 1 < stringSize)
            {
                outStr.push_back(ConvertUTF8ToUTF32_TwoBytes(firstByte, utf8String[index + 1]));
                index += 2;
            }
            else
            {
                throw std::exception("ConvertUTF8ToUTF32: Found two byte marker, but two bytes do not remain in string");
            }
        }
        else if ((firstByte & THREE_BYTE_MASK) == THREE_BYTE_VAL)
        {
            if (index + 2 < stringSize)
            {
                outStr.push_back(ConvertUTF8ToUTF32_ThreeBytes(firstByte, utf8String[index + 1], utf8String[index + 2]));
                index += 3;
            }
            else
            {
                throw std::exception("ConvertUTF8ToUTF32: Found three byte marker, but three bytes do not remain in string");
            }
        }
        else if ((firstByte & FOUR_BYTE_MASK) == FOUR_BYTE_VAL)
        {
            if (index + 3 < stringSize)
            {
                outStr.push_back(ConvertUTF8ToUTF32_FourBytes(firstByte, utf8String[index + 1], utf8String[index + 2], utf8String[index + 3]));
                index += 4;
            }
            else
            {
                throw std::exception("ConvertUTF8ToUTF32: Found four byte marker, but four bytes do not remain in string");
            }
        }
        else
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF8ToUTF32: Invalid UTF-8 starting byte encountered: %d", static_cast<uint32_t>(firstByte));
            throw std::exception(buf);
        }
    }

    return outStr;
}

std::string DX::Utf32ToUtf8(const std::u32string& utf32String)
{
    std::string utf8String;

    for (char32_t unicodeCharacter : utf32String)
    {
        if (unicodeCharacter >= 0x0 && unicodeCharacter <= 0x7F)
        {
            utf8String.push_back(static_cast<char>((unicodeCharacter & ONE_BYTE_CONT) | ONE_BYTE_MASK));
        }
        else if (unicodeCharacter >= 0x80 && unicodeCharacter <= 0x7FF)
        {
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 6) & TWO_BYTE_CONT) | TWO_BYTE_VAL));
            utf8String.push_back(static_cast<char>((unicodeCharacter & MUL_BYTE_CONT) | MUL_BYTE_VAL));
        }
        else if (unicodeCharacter >= 0x800 && unicodeCharacter <= 0x7FFF)
        {
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 12) & THREE_BYTE_CONT) | THREE_BYTE_VAL));
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 6) & MUL_BYTE_CONT) | MUL_BYTE_VAL));
            utf8String.push_back(static_cast<char>((unicodeCharacter & MUL_BYTE_CONT) | MUL_BYTE_VAL));
        }
        else if (unicodeCharacter >= 0x10000 && unicodeCharacter <= 0x10FFFF)
        {
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 18) & FOUR_BYTE_CONT) | FOUR_BYTE_VAL));
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 12) & MUL_BYTE_CONT) | MUL_BYTE_VAL));
            utf8String.push_back(static_cast<char>(((unicodeCharacter >> 6) & MUL_BYTE_CONT) | MUL_BYTE_VAL));
            utf8String.push_back(static_cast<char>((unicodeCharacter & MUL_BYTE_CONT) | MUL_BYTE_VAL));
        }
        else
        {
            char buf[128] = { 0 };
            sprintf_s(buf, "ConvertUTF32ToUTF8: Invalid UTF-32 character encountered: U+%x", static_cast<uint32_t>(unicodeCharacter));
            throw std::exception(buf);
        }
    }

    return utf8String;
}

std::string DX::ToLower(const std::string & utf8String)
{
    std::string lower = utf8String;
    ToLowerInPlace(lower);
    return lower;
}

void DX::ToLowerInPlace(std::string& utf8String)
{
    std::transform(
        utf8String.begin(),
        utf8String.end(),
        utf8String.begin(),
        [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    );
}

std::wstring DX::ToLower(const std::wstring & wideString)
{
    std::wstring lower = wideString;
    ToLowerInPlace(lower);
    return lower;
}

void DX::ToLowerInPlace(std::wstring & wideString)
{
    std::transform(
        wideString.begin(),
        wideString.end(),
        wideString.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::tolower(static_cast<wchar_t>(c))); }
    );
}

std::string DX::ToUpper(const std::string & utf8String)
{
    std::string lower = utf8String;
    ToUpperInPlace(lower);
    return lower;
}

void DX::ToUpperInPlace(std::string& utf8String)
{
    std::transform(
        utf8String.begin(),
        utf8String.end(),
        utf8String.begin(),
        [](char c) { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    );
}

std::wstring DX::ToUpper(const std::wstring & wideString)
{
    std::wstring lower = wideString;
    ToUpperInPlace(lower);
    return lower;
}

void DX::ToUpperInPlace(std::wstring & wideString)
{
    std::transform(
        wideString.begin(),
        wideString.end(),
        wideString.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::toupper(static_cast<wchar_t>(c))); }
    );
}
