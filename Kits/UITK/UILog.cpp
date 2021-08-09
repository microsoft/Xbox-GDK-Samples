//--------------------------------------------------------------------------------------
// File: UILog.cpp
//
// Authored by: ATG
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "UILog.h"

#include <atomic>
#include <chrono>

NAMESPACE_ATG_UITK_BEGIN

using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

std::atomic_int32_t s_uilogLineCounter(0);
time_point s_uilogStartTime = std::chrono::high_resolution_clock::now();

int32_t UILog::GetLineCounter()
{
    return s_uilogLineCounter.fetch_add(1);
}

int32_t UILog::GetAppLifetimeInMS()
{
    time_point now = std::chrono::high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - s_uilogStartTime);
    return int32_t(elapsed.count());
}

NAMESPACE_ATG_UITK_END

// THE FOLLOWING IS ONLY FOR TESTING THE MACROS COMPILATION ABILITY
#if UILOG_COMPILE_TESTS_ENABLE

NAMESPACE_ATG_UITK_BEGIN

#define UILOG_COMPILE_TEST(UILOG_XXX, Uniquefier) \
namespace UILOG_XXX ## Test ## Uniquefier                                                                           \
{                                                                                                                   \
    class UILOG_XXX ## TestClass ## Uniquefier                                                                      \
    {                                                                                                               \
        DECLARE_CLASS_LOG();                                                                                        \
                                                                                                                    \
        void Func();                                                                                                \
    };                                                                                                              \
                                                                                                                    \
    INITIALIZE_LOG_ ## UILOG_XXX ## (Test ## Uniquefier);                                                           \
    INITIALIZE_CLASS_LOG_ ## UILOG_XXX ## ( ## UILOG_XXX ## TestClass ## Uniquefier);                               \
                                                                                                                    \
    void UILOG_XXX ## TestClass ## Uniquefier::Func()                                                               \
    {                                                                                                               \
        UILOG_ ## UILOG_XXX ## _EXT(1024,                                                                           \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO");     \
        UILOG_ ## UILOG_XXX ##("FOO FOO FOO");                                                                      \
        UILOG_ ## UILOG_XXX ## _IF(true, "FOO FOO FOO");                                                            \
        UILOG_ ## UILOG_XXX ## _SCOPED("FOO", "FOO FOO FOO");                                                       \
        UILOG_ ## UILOG_XXX ## _FUNC("FOO FOO FOO");                                                                \
        UILOG_ ## UILOG_XXX ## _FUNC_EXT(1024,                                                                      \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO");     \
        UILOG_TRACE("FOO FOO FOO");                                                                                 \
        UILOG_DEBUG("FOO FOO FOO");                                                                                 \
        UILOG_INFO("FOO FOO FOO");                                                                                  \
        UILOG_WARN("FOO FOO FOO");                                                                                  \
        UILOG_ERROR("FOO FOO FOO");                                                                                 \
        UILOG_CRIT("FOO FOO FOO");                                                                                  \
    }                                                                                                               \
}

#pragma message("UILog compile test: CRIT")
UILOG_COMPILE_TEST(CRIT, 1);
#pragma message("UILog compile test: ERROR")
UILOG_COMPILE_TEST(ERROR, 2);
#pragma message("UILog compile test: WARN")
UILOG_COMPILE_TEST(WARN, 3);
#pragma message("UILog compile test: INFO")
UILOG_COMPILE_TEST(INFO, 4);
#pragma message("UILog compile test: DEBUG")
UILOG_COMPILE_TEST(DEBUG, 5);
#pragma message("UILog compile test: TRACE")
UILOG_COMPILE_TEST(TRACE, 6);

NAMESPACE_ATG_UITK_END

#endif
