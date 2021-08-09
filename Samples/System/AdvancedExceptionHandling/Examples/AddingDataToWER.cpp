//--------------------------------------------------------------------------------------
// AddingDataToWER.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"

// Need to include the WER api header for access to the WER specific functions, they are not part of Windows.h
#include <werapi.h>

// work around an incorrect partition setup for some of the WER defines in the GDK on the console
// this will be fixed in a future update
#ifdef _GAMING_XBOX
#define WER_FILE_DELETE_WHEN_DONE       1  // Delete the file once WER is done
#define WER_FILE_ANONYMOUS_DATA         2  // This file does not contain any PII
#define WER_FILE_COMPRESSED             4  // This file has been compressed using SQS
#endif

extern "C" LONG WINAPI AddDataToWERExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers);

namespace
{
    static const uint64_t c_memoryBlockSize = (64 * 1024) / sizeof(uint32_t);       // maximum size for a WER registered memory block
    FILE *g_logFile = nullptr;                                                      // Log file that will be included with the WER upload
    uint32_t *g_savedMemoryBlock = nullptr;                                         // The memory block that will be included with the WER upload
    uint32_t g_savedBlockWriteIndex = 0;                                            // The data block for this sample appends an increasing number on each log output
    std::wstring baseLogFileName(L"AddDataToWERLogFile.txt");                       // The name of the file that will be included with WER

    // when calling SetUnhandledExceptionFilter the previous filter is returned
    // this should be saved so that if needed it can be called after the new unhandled exception has finished executing
    // this allows the Windows Error Reporting system to process the error and upload the data to MS telemetry for processing
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionFilter = nullptr;

    // Helper macro to update the log file, the saved memory block, and the output message in one pass
#define UpdateLogs(str)                                                                 \
    {                                                                                   \
        Sample::m_testOutputMessage += str;                                             \
        if (g_logFile) fwprintf(g_logFile, str);                                        \
        g_savedMemoryBlock[g_savedBlockWriteIndex++] = g_savedBlockWriteIndex + 1;      \
    }

    // It's not possible to use structured exceptions inside a function that needs stack unwinding
    // For this reason operation that creates the exception is moved into its own function to allow the use of Structured Exception Handling
    // Structured Exception Handling is used in this sample to guarantee the ability to continue execution while walking through the sample
    void ForceException()
    {
        __try
        {
            char *fred = nullptr;
            *fred = 5;
        }
        __except (AddDataToWERExceptionFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

extern "C" LONG WINAPI AddDataToWERExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    UpdateLogs(L"Global Exception Handler called\n");

    // If possible the file that is going to be uploaded with the WER data should be closed before continuing to the WER system
    // This is not strictly needed, however the file should at least be flushed if possible from any title controlled cache
    if (g_logFile) fclose(g_logFile);
    g_logFile = nullptr;
    GenerateDump(exceptionPointers);

    return g_previousExceptionFilter(exceptionPointers);
}

void Sample::AddingDataToWER()
{
    m_lastTestRun = e_addingDataToWER;
    m_testOutputMessage = L"Starting Adding Data To Windows Error Reporting\n";

    std::wstring logFileName;
#ifdef _GAMING_DESKTOP
    {
        wchar_t currentDirectory[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDirectory);
        logFileName = currentDirectory;
        logFileName += L"\\";
    }
#else
    logFileName = L"d:\\";
#endif
    logFileName += baseLogFileName;

    _wfopen_s(&g_logFile, logFileName.c_str(), L"w");
    if (!g_logFile)
    {
        UpdateLogs(L"Failed to open file\n");
        goto cleanup;
    }

    g_savedMemoryBlock = new uint32_t[c_memoryBlockSize];
    memset(g_savedMemoryBlock, 0, c_memoryBlockSize);

    HRESULT hr;

    UpdateLogs(L"Starting run for Adding Data to WER example\n");

    // The name of the file is logged with WER using WerRegisterFile, the file does not have to be currently open
    // It's advisible to call this function early in your title and not wait until the exception has been caught
    // Waiting to create or register the file until the exception has been caught can cause other failures since the title is in an unknown state
    // It's also possible for some exceptions not to be caught by the title, for example a suspend failure, and the file should be included with WER
    hr = WerRegisterFile(logFileName.c_str(), WerRegFileTypeOther, WER_FILE_ANONYMOUS_DATA);
    if (!SUCCEEDED(hr))
    {
        UpdateLogs(L"Failed to register file\n");
        goto cleanup;
    }

    // WerRegisterMemoryBlock is currently not available in the GDK.
#ifndef _GAMING_XBOX
    hr = WerRegisterMemoryBlock(g_savedMemoryBlock, c_memoryBlockSize);

    if (!SUCCEEDED(hr))
    {
        UpdateLogs(L"Failed to register memory block\n");
        goto cleanup;
    }
#endif

    g_previousExceptionFilter = SetUnhandledExceptionFilter(AddDataToWERExceptionFilter);

    ForceException();

cleanup:
    if (g_previousExceptionFilter)
    {
        SetUnhandledExceptionFilter(g_previousExceptionFilter);
        g_previousExceptionFilter = nullptr;
    }
    if (g_logFile != nullptr)
    {
        UpdateLogs(L"Closing log file for Adding Data to WER example\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
    delete[] g_savedMemoryBlock;
    g_savedMemoryBlock = nullptr;
    m_testOutputMessage += L"Finished Unhandled Exception Filter example\n";
}
