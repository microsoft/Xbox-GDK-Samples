//--------------------------------------------------------------------------------------
// Log.h
//
// File, debug output and display logging support.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#pragma once

#include "ScopedLockWrappers.h"

#ifdef _DEBUG
#define ENABLE_LOGGING_TO_FILE
#endif

#define LOG_CREATE_NEW_ON_LAUNCH    1

// Logging for file and screen
namespace Log
{
    void Initialize();

    void ClearDisplayLog();

    // Adds message to the display log without any timestamp formatting
    void PushToDisplayLog( const std::wstring& message );
    void PushToDisplayLog( const std::string& message );
    void PushToDisplayLog( const wchar_t* message );
    void PushToDisplayLog( const char* message );

    // Writes to the debug log file (if enabled)
    void Write( const char* format, ...);
    void Write( const wchar_t* format, ... );

    // Writes to the debug log file AND sends formatted output to the in-game log display
    void WriteAndDisplay(const char* format, ...);
    void WriteAndDisplay( const wchar_t* format, ... );

    // Thread-safe log
    ATG::ScopedExclusiveLockWithPayload< std::vector<std::string> > GetDisplayLog_ScopedExclusiveLock();
    ATG::ScopedSharedLockWithPayload< std::vector<std::string> > GetDisplayLog_ScopedSharedLock();
};
