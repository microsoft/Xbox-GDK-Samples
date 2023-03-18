//--------------------------------------------------------------------------------------
// File: UnicodeRange.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <functional>

// HarfBuzz is required for text shaping
#include <hb.h>

#pragma once
namespace TextRenderer
{
    // All preset unicode ranges are obtained from:
    // https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/live/features/identity/user-profile/gamertags/live-modern-gamertags-unicode

    inline auto LatinSymbolsLambda = [](uint32_t character)
    {
        return (character >= 0x0021 && character <= 0x0026) ||
            (character >= 0x0028 && character <= 0x002F) ||
            (character >= 0x003A && character <= 0x0040) ||
            (character >= 0x005B && character <= 0x0060) ||
            (character >= 0x007B && character <= 0x007E) ||
            (character >= 0x00A0 && character <= 0x00BF);
    };

    inline auto LatinAlphaNumericLambda = [](uint32_t character)
    {
        return ((character == 0x0020) ||
            (character == 0x0027) ||
            (character >= 0x0030 && character <= 0x0039) ||
            (character >= 0x0041 && character <= 0x005A) ||
            (character >= 0x0061 && character <= 0x007A));
    };

    inline auto LatinSupplementalExtendedLambda = [](uint32_t character)
    {
        return ((character >= 0x00C0 && character <= 0x00F6) ||
            (character >= 0x00F8 && character <= 0x00FF) ||
            (character >= 0x0100 && character <= 0x017F));
    };

    inline auto KoreanLambda = [](uint32_t character)
    {
        return ((character >= 0x1100 && character <= 0x1112) ||
            (character >= 0x1161 && character <= 0x1175) ||
            (character >= 0x11A8 && character <= 0x11C2) ||
            (character >= 0xAC00 && character <= 0xD7A3));
    };

    inline auto JapaneseLambda = [](uint32_t character)
    {
        // Japanese kanji share the same unicode values as Chinese glyphs
        // If by chance you are trying to render both japanese and chinese characters
        // within the same multifont atlas, this will result in nondeterministic behavior.
        // You can fix this by forcing ALL japanese kanji to be rendered with the
        // chinese font file by removing the last line of this return statement.
        return ((character >= 0x3041 && character <= 0x3096) ||
            (character >= 0x30A1 && character <= 0x30FA)) ||
            ((character >= 0x4E00 && character <= 0x9FFF));
    };

    inline auto ChineseLambda = [](uint32_t character)
    {
        return ((character >= 0x4E00 && character <= 0x9FFF));
    };

    inline auto RussianLambda = [](uint32_t character)
    {
        return ((character >= 0x0400 && character <= 0x045F));
    };

    inline auto BengaliLambda = [](uint32_t character)
    {
        return ((character >= 0x0985 && character <= 0x09B9));
    };

    inline auto ThaiLambda = [](uint32_t character)
    {
        return ((character >= 0x0E01 && character <= 0x0E3A) ||
            (character >= 0x0E40 && character <= 0x0E4E));
    };

    inline auto GreekLambda = [](uint32_t character)
    {
        return ((character >= 0x0390 && character <= 0x03CE));
    };

    inline auto HindiLambda = [](uint32_t character)
    {
        return ((character >= 0x0900 && character <= 0x094F) ||
            (character >= 0x0966 && character <= 0x096F) ||
            (character >= 0x0671 && character <= 0x06D3) ||
            (character >= 0x06F0 && character <= 0x06F9));
    };

    inline auto ArabicLambda = [](uint32_t character)
    {
        return ((character >= 0x0620 && character <= 0x064A) ||
            (character >= 0x0660 && character <= 0x0669));
    };

    inline auto HebrewLambda = [](uint32_t character)
    {
        return ((character >= 0x05D0 && character <= 0x05EA));
    };

    inline auto PrivateUseLambda = [](uint32_t character)
    {
        return ((character >= 0xE000 && character <= 0xF8FF));
    };

    // Encapsulates all characters.
    inline auto OtherLambda = [](uint32_t character)
    {
        return character >= 0x0000;
    };

    enum class UnicodeRangeType
    {
        // Any character that does not fall within below ranges
        Other,

        // Symbols such as '!', ':', '?', etc
        LatinSymbols,

        // Latin alpha and numeric symbols, such as 'A', '1', ' ', etc.
        LatinAlphaNumberic,

        // Supplemental and extended Latin characters, such as accented Latin letters
        LatinSupplementalExtended,

        // Icons
        PrivateUseArea,

        // Language-specific ranges
        Korean,
        Japanese,
        Chinese,
        Russian,
        Bengali,
        Thai,
        Greek,
        Hindi,
        Arabic,
        Hebrew,
        COUNT
    };

    // The warning 26812 is ignored due to harfbuzz throwing this warning whenever
    // enums for hb_script_t or hb_direction_t are used. Harfbuzz doesn't use
    // enum classes, so this warning appears.
#pragma warning(push)
#pragma warning(disable: 26812)
    class UnicodeRange
    {
    public:
        UnicodeRange(
            std::function<bool(uint32_t)> unicodeRangeLambda,
            UnicodeRangeType rangeType,
            std::string languageCode,
            hb_script_t hbScript,
            hb_direction_t hbDirection) :
            m_unicodeRangeLambda(unicodeRangeLambda),
            m_unicodeRangeType(rangeType),
            m_languageCode(languageCode),
            m_hbScript(hbScript),
            m_hbDirection(hbDirection)
        {
        }

        ~UnicodeRange() = default;

        bool operator!=(UnicodeRange other) const
        {
            return m_unicodeRangeType != other.m_unicodeRangeType;
        }

        bool operator==(UnicodeRange other) const
        {
            return m_unicodeRangeType == other.m_unicodeRangeType;
        }

        bool operator<(const UnicodeRange& other) const
        {
            return m_unicodeRangeType < other.m_unicodeRangeType;
        }

        bool IsCharacterInRange(uint32_t character) const
        {
            return m_unicodeRangeLambda(character);
        }

        std::string GetLanguageCode()
        {
            return m_languageCode;
        }

        hb_script_t GetHBScript()
        {
            return m_hbScript;
        }

        hb_direction_t GetHBDirection()
        {
            return m_hbDirection;
        }

        UnicodeRangeType GetUnicodeRangeType()
        {
            return m_unicodeRangeType;
        }

    private:
        std::function<bool(uint32_t character)> m_unicodeRangeLambda;
        UnicodeRangeType                        m_unicodeRangeType;
        std::string                             m_languageCode;
        hb_script_t                             m_hbScript;
        hb_direction_t                          m_hbDirection;
    };

    namespace UnicodeRangeInfo
    {
        extern const UnicodeRange LatinSymbols;
        extern const UnicodeRange LatinAlphaNumeric;
        extern const UnicodeRange LatinSupplement;
        extern const UnicodeRange Korean;
        extern const UnicodeRange Japanese;
        extern const UnicodeRange Chinese;
        extern const UnicodeRange Russian;
        extern const UnicodeRange Bengali;
        extern const UnicodeRange Thai;
        extern const UnicodeRange Greek;
        extern const UnicodeRange Hindi;
        extern const UnicodeRange Arabic;
        extern const UnicodeRange Hebrew;
        extern const UnicodeRange PrivateUse;
        extern const UnicodeRange Other;
    }
#pragma warning(pop)
}
