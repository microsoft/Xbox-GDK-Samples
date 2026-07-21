//--------------------------------------------------------------------------------------
// ThreadEnum.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Used to provide functionality to enumerate all of the active threads on the system
// This is only useful on Xbox, for other platforms use the provided functions in toolhelp

#include <functional>
#include <cstdint>

namespace ATG
{
    typedef std::function<bool(uint32_t processID, uint32_t threadID)> EnumThreadsCallbackFunc;    // return true if continue processing
    void EnumerateThreads(const EnumThreadsCallbackFunc& callbackFunc);
}
