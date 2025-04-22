//------------------------------------------------------------------------------
// SampleAssert.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

#include <stdio.h>

#ifndef SAMPLE_ASSERT_MESSAGE

    // Define a special macro that replaces `while(0)` so that it doesn't produce warnings
    #if defined(__clang__)
        #define SAMPLE_ASSERT_WHILE0 while(0)
    #elif defined(_MSC_VER)
        #define SAMPLE_ASSERT_WHILE0 __pragma(warning(push)) __pragma(warning(disable : 4127)) while(0) __pragma(warning(push))
    #endif

    #ifdef _DEBUG
        #define SAMPLE_ASSERT_TO_STRING(s) #s
        #define SAMPLE_ASSERT_MACRO_VALUE_TO_STRING(s) SAMPLE_ASSERT_TO_STRING(s)

        #define SAMPLE_ASSERT_MESSAGE(cond, format, ...)\
            do                                          \
            {                                           \
                if (!(cond))                            \
                {                                       \
                    char buffer[256];                   \
                    _snprintf_s(buffer, _countof(buffer), _countof(buffer) - 1, format " failed \n\tIn " __FILE__ ":" SAMPLE_ASSERT_MACRO_VALUE_TO_STRING(__LINE__) "\n", ## __VA_ARGS__);\
                    OutputDebugStringA(buffer);         \
                    __debugbreak();                     \
                }                                       \
            }                                           \
            SAMPLE_ASSERT_WHILE0

    #else
        #define SAMPLE_ASSERT_MESSAGE(cond, format, ...)\
            do                                          \
            {                                           \
                (void)(1 ? 0 : (cond));                 \
            }                                           \
            SAMPLE_ASSERT_WHILE0

    #endif
#endif

#ifndef SAMPLE_ASSERT
#define SAMPLE_ASSERT(cond) SAMPLE_ASSERT_MESSAGE(cond, "Assertion '" #cond "'")
#endif
