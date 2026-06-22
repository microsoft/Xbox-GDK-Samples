//--------------------------------------------------------------------------------------
// Log.cpp
//
// File, debug output and display logging support - implementation file.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Log.h"
#include "..\Helpers\UTF8Helper.h"
#include <mutex>
#include "malloc.h"
#include <Shlobj.h>

namespace Log
{
   SRWLOCK g_logLock = SRWLOCK_INIT;
   std::vector<std::string> g_displayLog;
}

namespace
{
    const size_t    c_logBufferSize = 8192;
    const char*     c_logEntryPrefix = u8"WORDGAME: ";
#ifdef ENABLE_LOGGING_TO_FILE
    const wchar_t*  c_logFilename = L"WordGameDebugLog.txt";
#endif
    std::wstring    g_logFilePath;
    std::mutex      g_logMutex;

    // Writes message to debug output, prefixing message with c_logEntryPrefix
    // If ENABLE_LOGGING_TO_FILE is defined, writes message to c_logFilename, prefixing with timestamp
    // Returns new string containing message prefixed with timestamp
    std::string WriteToDebugOutputAndLogFile(const char* message)
    {
        std::lock_guard<std::mutex> lock(g_logMutex);

        OutputDebugStringA( c_logEntryPrefix);
        OutputDebugStringA( message );

        // note the log time
        SYSTEMTIME localTime;
        GetLocalTime(&localTime);

        char timeStr[100];
        sprintf_s(timeStr, 100, "%02u:%02u:%02u.%03u ",
            localTime.wHour,
            localTime.wMinute,
            localTime.wSecond,
            localTime.wMilliseconds); // format: "hh:mm:ss.ms"

#ifdef ENABLE_LOGGING_TO_FILE
        FILE *file = nullptr;

        errno_t err = _wfopen_s(
            &file,
            g_logFilePath.c_str(),
            L"at+"
        );

        if (err != 0)
        {
            std::string err_msg("ERROR: Unable to open log file (code ");
            err_msg += std::to_string(err);
            err_msg += ")\n";
            OutputDebugStringA(err_msg.c_str());
        }
        else
        {
            auto fileStr = std::to_string(GetCurrentThreadId());
            fileStr += " \t";
            fileStr += timeStr;
            fileStr += message;

            fwrite(
                (void*)fileStr.c_str(),
                sizeof(char),
                fileStr.length(),
                file
            );

            fflush(file);
            fclose(file);
        }
#endif

        // assemble return line
        std::string returnLine(timeStr);
        returnLine += message;

        return returnLine;
    }


    std::string WriteToDebugOutputAndLogFile( const wchar_t* message )
    {
       assert( message != nullptr && "string must not be null" );
       size_t numwchar = wcslen( message );
       size_t tempBufSize = ( ATG::Text::MAX_BYTES_PER_UTF8_CHAR * numwchar ) + 1;
       auto buf = (char*) alloca( tempBufSize );

       ::WideCharToMultiByte( CP_UTF8, 0, message, int(numwchar + 1), buf, int(tempBufSize), nullptr, nullptr );

       return WriteToDebugOutputAndLogFile( buf );
    }

} // end unnamed namespace


void Log::Initialize()
{
#ifdef ENABLE_LOGGING_TO_FILE

    // Get log path
    wchar_t* userPath = nullptr;
    DX::ThrowIfFailed(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &userPath));
    std::wstring path(userPath);
    CoTaskMemFree(userPath);

    // Create directories
    path += L"\\Microsoft";
    CreateDirectoryW(path.c_str(), nullptr);
    path += L"\\ATGSamples";
    CreateDirectoryW(path.c_str(), nullptr);
    path += L"\\GameSave_Desktop";
    CreateDirectoryW(path.c_str(), nullptr);

    // set log path
    path += L"\\";
    path += c_logFilename;
    g_logFilePath = path;
    OutputDebugStringW(L"Log file path: ");
    OutputDebugStringW(g_logFilePath.c_str());
    OutputDebugStringW(L"\n");

    // Create intermediate folders if they don't exist


#if LOG_CREATE_NEW_ON_LAUNCH == 1
    // delete previous log file, ignoring failures
    _wremove(g_logFilePath.c_str());
#endif

#endif
}

void Log::ClearDisplayLog()
{
   Log::GetDisplayLog_ScopedExclusiveLock()->clear();
}

void Log::PushToDisplayLog( const std::wstring& message )
{
   Log::GetDisplayLog_ScopedExclusiveLock()->push_back( ATG::Text::ToUTF8String( message ) );
}

void Log::PushToDisplayLog( const wchar_t* message )
{
   Log::GetDisplayLog_ScopedExclusiveLock()->push_back( ATG::Text::ToUTF8String( message ) );
}

void Log::PushToDisplayLog( const char* message )
{
   Log::GetDisplayLog_ScopedExclusiveLock()->push_back( std::string( message ) );
}

void Log::PushToDisplayLog(const std::string& message)
{
   Log::GetDisplayLog_ScopedExclusiveLock()->push_back( message ); 
}

void Log::Write( const char* format, ... )
{
   static char msgbuffer[ c_logBufferSize ];

   va_list args;
   va_start( args, format );
   vsprintf_s( msgbuffer, format, args );
   va_end( args );

   WriteToDebugOutputAndLogFile( msgbuffer );
}

void Log::Write( const wchar_t* format, ... )
{
   static wchar_t msgbuffer[ c_logBufferSize ];

   va_list args;
   va_start( args, format );
   vswprintf( msgbuffer, c_logBufferSize, format, args );
   va_end( args );

   WriteToDebugOutputAndLogFile( msgbuffer );
}

void Log::WriteAndDisplay( const wchar_t* format, ... )
{
   static wchar_t msgbuffer[ c_logBufferSize ];

   va_list args;
   va_start( args, format );
   vswprintf( msgbuffer, c_logBufferSize, format, args );
   va_end( args );

   PushToDisplayLog( WriteToDebugOutputAndLogFile( msgbuffer ) );
}

ATG::ScopedExclusiveLockWithPayload<std::vector<std::string> > Log::GetDisplayLog_ScopedExclusiveLock()
{
   return ATG::ScopedExclusiveLockWithPayload< std::vector<std::string> >(g_logLock, Log::g_displayLog );
}

ATG::ScopedSharedLockWithPayload<std::vector<std::string> > Log::GetDisplayLog_ScopedSharedLock()
{
   return ATG::ScopedSharedLockWithPayload< std::vector<std::string> >(g_logLock, Log::g_displayLog);
}

void Log::WriteAndDisplay( const char* format, ... )
{
   static char msgbuffer[ c_logBufferSize ];

   va_list args;
   va_start( args, format );
   vsprintf_s( msgbuffer, format, args );
   va_end( args );

   PushToDisplayLog( WriteToDebugOutputAndLogFile( msgbuffer ) );
}
