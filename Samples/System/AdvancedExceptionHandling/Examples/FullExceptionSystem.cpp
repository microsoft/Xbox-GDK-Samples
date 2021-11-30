//--------------------------------------------------------------------------------------
// FullExceptionSystem.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"
#include "SharedMemory\SharedMemory.h"
#include <processthreadsapi.h>

// Need to include the WER api header for access to the WER specific functions, they are not part of Windows.h
#include "werapi.h"

// work around an incorrect partition setup for some of the WER functions in the GDK on the console
// this will be fixed in a future update
#ifdef _GAMING_XBOX
typedef HRESULT(WINAPI* PF_WerRegisterMemoryBlock)(_In_ PVOID pvAddress, _In_ DWORD dwSize);

#define WER_FILE_DELETE_WHEN_DONE       1  // Delete the file once WER is done
#define WER_FILE_ANONYMOUS_DATA         2  // This file does not contain any PII
#define WER_FILE_COMPRESSED             4  // This file has been compressed using SQS
#endif

using namespace SharedMemory;

extern "C" LONG WINAPI GlobalExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers);
void ThreadWorkingFullExceptionFunction();
void ThreadEntryFullExceptionFunction();

namespace
{
    STARTUPINFO g_startInfo;                                            // The startup data used when creating the process to write the crash dumps from our process
    PROCESS_INFORMATION g_procInfo;                                     // The details on the external process used to write out crash dumps
    std::wstring g_baseDumpFileName(L"FullException");                        // The default name used for the crash dump
    std::wstring g_outputDirectory;

    // when calling SetUnhandledExceptionFilter the previous filter is returned
    // this should be saved so that if needed it can be called after the new unhandled exception has finished executing
    // this allows the Windows Error Reporting system to process the error and upload the data to MS telemetry for processing
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionFilter = nullptr;

    constexpr uint64_t c_memoryBlockSize = (64 * 1024) / sizeof(uint32_t);       // maximum size for a WER registered memory block
    FILE* g_logFile = nullptr;                                                      // Log file that will be included with the WER upload
    uint32_t* g_savedMemoryBlock = nullptr;                                         // The memory block that will be included with the WER upload
    uint32_t g_savedBlockWriteIndex = 0;                                            // The data block for this sample appends an increasing number on each log output
    std::wstring g_baseLogFileName(L"FullExceptionLogFile.txt");                       // The name of the file that will be included with WER

    // Helper macro to update the log file, the saved memory block, and the output message in one pass
#define UpdateLogs(str)                                                                 \
    {                                                                                   \
        Sample::m_testOutputMessage += str;                                             \
        if (g_logFile) fwprintf(g_logFile, str);                                                       \
        g_savedMemoryBlock[g_savedBlockWriteIndex++] = g_savedBlockWriteIndex + 1;      \
    }

    // Thread function and list of existing crash dumps found for upload
    std::vector<std::wstring> g_foundCrashDumps;
    void UploadCrashDumpThread()
    {
        for (auto& iter : g_foundCrashDumps)
        {
            Sample::m_testOutputMessage += L"Found and uploaded dump file: ";
            Sample::m_testOutputMessage += iter;
            Sample::m_testOutputMessage += L"\n";

            // Upload the file, when the upload is complete delete the file.
            // If something happens where an upload is interrupted the original file is still on the disk and the next attempt will try again.
            DeleteFileW(iter.c_str());
        }
    }

    bool SetupExceptionSystem()
    {
        // the shared memory block should be created early in the startup time of the main process
        // Waiting until after the exception has been fired could easily have failures, for example if there is not enough memory available
        if (!SharedMemory::InitSharedMemory())
        {
            Sample::m_testOutputMessage += L"Failed to create shared memory\n";
            return false;
        }
        memset(&g_startInfo, 0, sizeof(g_startInfo));
        memset(&g_procInfo, 0, sizeof(g_procInfo));
        g_startInfo.cb = sizeof(g_startInfo);

        wchar_t fullPath[1024] = {};
        GetModuleFileNameW(nullptr, fullPath, 1024);
        std::wstring exeName = fullPath;
        exeName = exeName.substr(exeName.find_last_of(L"\\") + 1);

#ifdef _GAMING_DESKTOP
        {
            wchar_t currentDirectory[MAX_PATH];
            GetCurrentDirectoryW(MAX_PATH, currentDirectory);
            g_outputDirectory = currentDirectory;
            g_outputDirectory += L"\\";
        }
#else
        g_outputDirectory = L"d:\\";
#endif
        std::wstring fullDumpName;
        std::wstring logFileName;
        logFileName = g_outputDirectory + g_baseLogFileName;
        fullDumpName = g_outputDirectory + g_baseDumpFileName;

        g_sharedMemory->SetBaseDumpName(fullDumpName);
        g_sharedMemory->SetApplicationName(exeName);

        // The shared memory block should be created early in the startup time of the main process
        // Waiting until after the exception has been fired could easily have failures, for example if there is not enough memory available
        if (!CreateProcessW(L"OutOfProcDumpTool.exe", nullptr, nullptr, nullptr, TRUE, 0 /*CREATE_NO_WINDOW | DETACHED_PROCESS*/, nullptr, nullptr, &g_startInfo, &g_procInfo))
        {
            Sample::m_testOutputMessage += L"Failed to create secondary process\n";
            return false;
        }

        g_savedMemoryBlock = new uint32_t[c_memoryBlockSize];
        memset(g_savedMemoryBlock, 0, c_memoryBlockSize);
        _wfopen_s(&g_logFile, logFileName.c_str(), L"w");

        UpdateLogs(L"Starting run for Adding Data to WER sample\n");

        // The name of the file is logged with WER using WerRegisterFile, the file does not have to be currently open
        // It's advisible to call this function early in your title and not wait until the exception has been caught
        // Waiting to create or register the file until the exception has been caught can cause other failures since the title is in an unknown state
        // It's also possible for some exceptions not to be caught by the title, for example a suspend failure, and the file should be included with WER
        HRESULT hr = WerRegisterFile(logFileName.c_str(), WerRegFileTypeOther, WER_FILE_ANONYMOUS_DATA);
        if (!SUCCEEDED(hr))
        {
            UpdateLogs(L"Failed to register file\n");
            return false;
        }

#ifdef _GAMING_XBOX
        // WerRegisterMemoryBlock is currently not part of the xgameplatform.lib umbrella library
        // As a workaround, we import it directly from the appropriate API set.
        {
            static HMODULE s_hmod = nullptr;
            if (!s_hmod)
            {
                s_hmod = LoadLibraryW(L"api-ms-win-core-windowserrorreporting-l1-1-0.dll");
            }
            assert(s_hmod != nullptr);
            auto proc = reinterpret_cast<PF_WerRegisterMemoryBlock>(reinterpret_cast<void*>(GetProcAddress(s_hmod, "WerRegisterMemoryBlock")));
            assert(proc != nullptr);
            hr = proc(g_savedMemoryBlock, c_memoryBlockSize);
        }
#else
        hr = WerRegisterMemoryBlock(g_savedMemoryBlock, c_memoryBlockSize);
#endif

        if (!SUCCEEDED(hr))
        {
            UpdateLogs(L"Failed to register memory block\n");
            return false;
        }
        g_previousExceptionFilter = SetUnhandledExceptionFilter(GlobalExceptionFilter);
        return true;
    }

    void CleanupExceptionSystem()
    {
        if (g_previousExceptionFilter)
        {
            SetUnhandledExceptionFilter(g_previousExceptionFilter);
            g_previousExceptionFilter = nullptr;
        }

        g_sharedMemory->shutdown = true;
        SetEvent(g_sharedStartEvent);

        WaitForSingleObject(g_procInfo.hProcess, INFINITE);
        SharedMemory::CleanupSharedMemory();

        if (g_logFile != nullptr)
        {
            UpdateLogs(L"Closing log file for Full Exception System example\n");
            fclose(g_logFile);
            g_logFile = nullptr;
        }
        delete[] g_savedMemoryBlock;
        g_savedMemoryBlock = nullptr;
    }
}

extern "C" LONG WINAPI GlobalExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    Sample::m_testOutputMessage += L"Full Exception System Exception Handler called\n";

    // If possible the file that is going to be uploaded with the WER data should be closed before continuing to the WER system
    // This is not strictly needed, however the file should at least be flushed if possible from any title controlled cache
    if (g_logFile) fclose(g_logFile);
    g_logFile = nullptr;

    // The title is currently suspending or resuming when an exception happened
    // In this case title exception handling code should not be executing or there is a possiblity of extending past the timer
    // If the work extends beyond the timer for suspend/resume the title will be forcibly terminated in the middle of handling the exception
    if (!Sample::s_isSuspending)
    {
        // Save out the exception data to the shared memory block
        g_sharedMemory->threadId = GetCurrentThreadId();
        g_sharedMemory->pointers = exceptionPointers;
        g_sharedMemory->miniDumpType = MiniDumpNormal;

        // Fire the event that tells the second process to create a crash dumps
        SetEvent(g_sharedStartEvent);

        // Wait for the second process to finish writing out a crash dump
        WaitForSingleObject(g_sharedFinishedEvent, INFINITE);
    }

    // Possible return values from the unhandled exception filter
    // EXCEPTION_EXECUTE_HANDLER - directly proceed the top level exception handler in the system, passes control to MS crash reporting telemetry and terminates the title
    // EXCEPTION_CONTINUE_SEARCH - Proceed with the normal execution for an unhandled exception, for instance on Windows putting up a crash dialog
    // EXCEPTION_CONTINUE_EXECUTION - Continue execution at the point of the exception

    // Note: This sample is returning EXCEPTION_CONTINUE_EXECUTION to allow the sample to continue.
    // This is not the recommended pattern for a title.
    return  EXCEPTION_CONTINUE_EXECUTION;

    // The recommended action is to call the previous unhandled exception filter, returned by the call to SetUnhandledExceptionFilter
    // In this case Windows Error Reporting would be called to save a crash dump, upload it to the Microsoft Servers, and terminate the title
    //return g_previousExceptionFilter(exceptionPointers);
}

void HandlePendingMiniDumps(const std::wstring& basePath)
{
    // Scan the drive finding all the current crash dump files and add them to the upload list
    WIN32_FIND_DATAW findData;
    std::wstring searchPath(basePath);
    searchPath += L"*.dmp";
    HANDLE searchHandle = FindFirstFileW(searchPath.c_str(), &findData);
    if (searchHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring foundName = findData.cFileName;
            if (findData.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY)
            {
                std::wstring fullName = basePath;
                fullName += foundName;

                g_foundCrashDumps.push_back(fullName);

                continue;
            }
        } while (FindNextFileW(searchHandle, &findData));
        FindClose(searchHandle);
    }

    if (g_foundCrashDumps.size() != 0)
    {
        // Once all the crash dumps have been found startup a background thread to process them while the title can continue running
        std::thread* uploadThread = new std::thread(UploadCrashDumpThread);

        // continue running normal game code

        uploadThread->join();
        delete uploadThread;
    }
    else
    {
        Sample::m_testOutputMessage += L"Did not find any crash dumps for upload\n";
    }
}

// Entry point for any title created thread should be protected with a structured exception handler
// This will allow each thread to capture the exception for processing before the unhandled exception filter is called
// If the title doesn't care about capturing exceptions at the thread level it's safe to remove this structured exception wrapper
void ThreadEntryFullExceptionFunction()
{
    Sample::m_testOutputMessage += L"Starting Thread Entry FullException Function\n";
    __try
    {
        // The title can do any code it wants here, it does not have to place all its main code in a second function
        // The part to remember is that you can't have a structured exception handler in a function that requires stack unwinding.
        ThreadWorkingFullExceptionFunction();
    }

    // It's important to return EXCEPTION_CONTINUE_SEARCH from the __except block if the title cannot continue execution after the exception
    // the reason is to allow the unhandled exception filter to be called and eventually call the Microsoft exception handler
    // This allows the Microsoft crash telemetry system to collect the data and upload it to the Microsoft servers for processing
    // Titles are given access to this data for their own use if desired
    __except (GlobalExceptionFilter(GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION)
    {                                                   // This should be EXCEPTION_CONTINUE_SEARCH to allow MS telemetry to collect crash data
        Sample::m_testOutputMessage += L"In the Thread Exception __except block\n";
    }

    Sample::m_testOutputMessage += L"Finished Thread Entry FullException Function\n";
}

// the next function to call from the Thread entry point, this would be where the main work would be performed
// See the notes on the Thread Entry function above for the reason to split these two functions
void ThreadWorkingFullExceptionFunction()
{
    Sample::m_testOutputMessage += L"Starting Thread Working FullException Function\n";

    // the need for this C++ try/catch block is dependent on the title and its usage of C++ exception handling
    // Still it's advisable to have a block like this to catch possible unknown C++ exceptions that might be an error for your title
    // Note: There is no runtime overhead for C++ exception handling in the x64 environment
    // The overhead associated with C++ exception handling happens at exception time. There is a static function frame table that
    // is stored with your title. This is used for frame unwinding, including possible stack unwinding by the title at exception time
    try
    {
        // do some work that may throw and exception
        throw std::bad_alloc();
    }
    catch (const std::exception& exception)
    {
        if (dynamic_cast<const std::bad_alloc*>(&exception) != nullptr)
        {
            Sample::m_testOutputMessage += L"Caught a C++ bad_alloc exception\n";
            RaiseException(ERROR_OUTOFMEMORY, 0, 0, nullptr);
        }
        else
        {
            Sample::m_testOutputMessage += L"Caught a general C++ exception\n";
            RaiseException(ERROR_UNIDENTIFIED_ERROR, 0, 0, nullptr);
        }
    }
    Sample::m_testOutputMessage += L"Finished Thread Working FullException Function\n";
}

void Sample::FullExceptionSystem()
{
    m_lastTestRun = e_fullExceptionSystem;
    m_testOutputMessage = L"Starting Full Exception Handling Example\n";
    if (SetupExceptionSystem())
    {
        // Start the upload of any crash dumps from previous runs of the title
        HandlePendingMiniDumps(g_outputDirectory);

        // Start the general title execution, in this case it's a single thread
        std::thread* workerThread = new std::thread(ThreadEntryFullExceptionFunction);
        if (workerThread)
        {
            workerThread->join();
            delete workerThread;
        }

        CleanupExceptionSystem();
    }
    else
    {
        m_testOutputMessage += L"Failed to setup exception system\n";
    }

    m_testOutputMessage += L"Finished Full Exception Handling Example\n";
}
