//--------------------------------------------------------------------------------------
// File: UnicodeRange.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UnicodeRange.h"

namespace TextRenderer
{
    namespace UnicodeRangeInfo
    {
        const UnicodeRange LatinSymbols = { LatinSymbolsLambda, UnicodeRangeType::LatinSymbols, "en", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange LatinAlphaNumeric = { LatinAlphaNumericLambda,  UnicodeRangeType::LatinAlphaNumberic, "en", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange LatinSupplement = { LatinSupplementalExtendedLambda,  UnicodeRangeType::LatinSupplementalExtended, "en", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Korean = { KoreanLambda, UnicodeRangeType::Korean, "ko", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Japanese = { JapaneseLambda, UnicodeRangeType::Japanese, "ja", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Chinese = { ChineseLambda, UnicodeRangeType::Chinese, "zh", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Russian = { RussianLambda, UnicodeRangeType::Russian, "ru", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Bengali = { BengaliLambda, UnicodeRangeType::Bengali, "bn", HB_SCRIPT_BENGALI , HB_DIRECTION_LTR };
        const UnicodeRange Thai = { ThaiLambda, UnicodeRangeType::Thai, "th", HB_SCRIPT_THAI, HB_DIRECTION_LTR };
        const UnicodeRange Greek = { GreekLambda, UnicodeRangeType::Greek, "el", HB_SCRIPT_GREEK, HB_DIRECTION_LTR };
        const UnicodeRange Hindi = { HindiLambda, UnicodeRangeType::Hindi, "hi", HB_SCRIPT_DEVANAGARI, HB_DIRECTION_LTR };
        const UnicodeRange Arabic = { ArabicLambda, UnicodeRangeType::Arabic, "ar", HB_SCRIPT_ARABIC, HB_DIRECTION_RTL };
        const UnicodeRange Hebrew = { HebrewLambda, UnicodeRangeType::Hebrew, "he", HB_SCRIPT_HEBREW, HB_DIRECTION_RTL };
        const UnicodeRange PrivateUse = { PrivateUseLambda, UnicodeRangeType::PrivateUseArea, "en", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
        const UnicodeRange Other = { OtherLambda, UnicodeRangeType::Other, "en", HB_SCRIPT_UNKNOWN, HB_DIRECTION_LTR };
    }
}
