//--------------------------------------------------------------------------------------
// Logger_test.cpp
//
// Compilation tests for the Logger class
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Logger.h"

#pragma message("*** LOGGER_TEST.CPP IS FOR TESTING ONLY. PLEASE DO NOT INCLUDE IN ANY PROJECT! ***")

// THE FOLLOWING IS ONLY FOR TESTING THE MACROS COMPILATION ABILITY
#if LOGGER_COMPILE_TESTS_ENABLE

namespace NotATG {

#define LOGGER_COMPILE_TEST(LOGGER_XXX, Uniquefier) \
namespace LOGGER_XXX ## Test ## Uniquefier                                                                          \
{                                                                                                                   \
    class LOGGER_XXX ## TestClass ## Uniquefier                                                                     \
    {                                                                                                               \
        DECLARE_CLASS_LOG();                                                                                        \
                                                                                                                    \
        void Func();                                                                                                \
    };                                                                                                              \
                                                                                                                    \
    INITIALIZE_LOG_ ## LOGGER_XXX ## (Test ## Uniquefier);                                                          \
    INITIALIZE_CLASS_LOG_ ## LOGGER_XXX ## ( ## LOGGER_XXX ## TestClass ## Uniquefier);                             \
                                                                                                                    \
    void LOGGER_XXX ## TestClass ## Uniquefier::Func()                                                              \
    {                                                                                                               \
        LOGGER_ ## LOGGER_XXX ## _EXT(1024,                                                                         \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO");     \
        LOGGER_ ## LOGGER_XXX ##("FOO FOO FOO");                                                                    \
        LOGGER_ ## LOGGER_XXX ## _IF(true, "FOO FOO FOO");                                                          \
        LOGGER_ ## LOGGER_XXX ## _SCOPED("FOO", "FOO FOO FOO");                                                     \
        LOGGER_ ## LOGGER_XXX ## _FUNC("FOO FOO FOO");                                                              \
        LOGGER_ ## LOGGER_XXX ## _FUNC_EXT(1024,                                                                    \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO"       \
            "FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO FOO");     \
        LOGGER_TRACE("FOO FOO FOO");                                                                                \
        LOGGER_DEBUG("FOO FOO FOO");                                                                                \
        LOGGER_INFO("FOO FOO FOO");                                                                                 \
        LOGGER_WARN("FOO FOO FOO");                                                                                 \
        LOGGER_ERROR("FOO FOO FOO");                                                                                \
        LOGGER_CRIT("FOO FOO FOO");                                                                                 \
    }                                                                                                               \
}

#pragma message("Logger compile test: CRIT")
LOGGER_COMPILE_TEST(CRIT, 1);
#pragma message("Logger compile test: ERROR")
LOGGER_COMPILE_TEST(ERROR, 2);
#pragma message("Logger compile test: WARN")
LOGGER_COMPILE_TEST(WARN, 3);
#pragma message("Logger compile test: INFO")
LOGGER_COMPILE_TEST(INFO, 4);
#pragma message("Logger compile test: DEBUG")
LOGGER_COMPILE_TEST(DEBUG, 5);
#pragma message("Logger compile test: TRACE")
LOGGER_COMPILE_TEST(TRACE, 6);

};

#endif
