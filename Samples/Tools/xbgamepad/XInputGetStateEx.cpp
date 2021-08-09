//--------------------------------------------------------------------------------------
// XInputGetStateEx.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <Windows.h>
#include "XInputGetStateEx.h"

XInputGetStateExFnPtr XInputGetStateEx = nullptr;

bool SetupXInputGetStateEx()
{
    auto xinput = LoadLibraryEx(L"xinput1_4.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (xinput)
    {
        // The XInputGetStateEx private interface is exactly like the public function, but does not mask off the Guide/Nexus button.
        XInputGetStateEx = (XInputGetStateExFnPtr)GetProcAddress(xinput, MAKEINTRESOURCEA(100));
    }

    return (XInputGetStateEx != nullptr);
}
