//--------------------------------------------------------------------------------------
// OutOfProcHandler.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"
#include "SharedMemory\SharedMemory.h"
#include <processthreadsapi.h>

using namespace SharedMemory;

extern "C" LONG WINAPI OutofProcExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers);

namespace
{
    STARTUPINFO g_startInfo;                                            // The startup data used when creating the process to write the crash dumps from our process
    PROCESS_INFORMATION g_procInfo;                                     // The details on the external process used to write out crash dumps
    std::wstring baseDumpFileName(L"OutOfProc");                        // The default name used for the crash dump

    // when calling SetUnhandledExceptionFilter the previous filter is returned
    // this should be saved so that if needed it can be called after the new unhandled exception has finished executing
    // this allows the Windows Error Reporting system to process the error and upload the data to MS telemetry for processing
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionFilter = nullptr;

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
        __except (OutofProcExceptionFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

// The exception filter for this sample saves the exception data to a shared memory block and then signals an Event to tell the second process to write the crash dump
// Once the crash dump has been written the second process signal an Event that says it's done and this process can continue execution
extern "C" LONG WINAPI OutofProcExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    Sample::m_testOutputMessage += L"Out of Proc Exception Handler called\n";

    // Save out the exception data to the shared memory block
    g_sharedMemory->threadId = GetCurrentThreadId();
    g_sharedMemory->pointers = exceptionPointers;
    g_sharedMemory->miniDumpType = MiniDumpNormal;

    // Fire the event that tells the second process to create a crash dumps
    SetEvent(g_sharedStartEvent);

    // Wait for the second process to finish writing out a crash dump
    WaitForSingleObject(g_sharedFinishedEvent, INFINITE);

    Sample::m_testOutputMessage += L"Out of Proc Exception Handler completed\n";

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

void Sample::OutOfProcHandler()
{
    m_testOutputMessage = L"Starting Out of Proc example\n";
    m_lastTestRun = e_outOfProcHandler;

    // the shared memory block should be created early in the startup time of the main process
    // Waiting until after the exception has been fired could easily have failures, for example if there is not enough memory available
    if (!SharedMemory::InitSharedMemory())
    {
        m_testOutputMessage += L"Failed to create shared memory\n";
        return;
    }
    memset(&g_startInfo, 0, sizeof(g_startInfo));
    memset(&g_procInfo, 0, sizeof(g_procInfo));
    g_startInfo.cb = sizeof(g_startInfo);

    wchar_t fullPath[1024] = {};
    GetModuleFileNameW(nullptr, fullPath, 1024);
    std::wstring exeName = fullPath;
    exeName = exeName.substr(exeName.find_last_of(L"\\") + 1);
    std::wstring fullDumpName;

#ifdef _GAMING_DESKTOP
    {
        wchar_t currentDirectory[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDirectory);
        fullDumpName = currentDirectory;
        fullDumpName += L"\\";
    }
#else
    fullDumpName = L"d:\\";
#endif
    fullDumpName += baseDumpFileName;

    g_sharedMemory->SetBaseDumpName(fullDumpName);
    g_sharedMemory->SetApplicationName(exeName);

    // The shared memory block should be created early in the startup time of the main process
    // Waiting until after the exception has been fired could easily have failures, for example if there is not enough memory available
    if (!CreateProcessW(L"OutOfProcDumpTool.exe", nullptr, nullptr, nullptr, TRUE, CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr, nullptr, &g_startInfo, &g_procInfo))
    {
        m_testOutputMessage += L"Failed to create secondary process\n";
        return;
    }

    m_testOutputMessage += L"Created secondary process\n";
    g_previousExceptionFilter = SetUnhandledExceptionFilter(OutofProcExceptionFilter);

    ForceException();

    if (g_previousExceptionFilter)
    {
        SetUnhandledExceptionFilter(g_previousExceptionFilter);
        g_previousExceptionFilter = nullptr;
    }

    // This particular sample and external OutOfProcDumpTool can continue to write crash dumps versus just terminating the process
    // Because of this it also supports the ability to be told to terminate by setting the shutdown flag and telling it to run again
    g_sharedMemory->shutdown = true;
    SetEvent(g_sharedStartEvent);
    WaitForSingleObject(g_procInfo.hProcess, INFINITE);

    SharedMemory::CleanupSharedMemory();
    m_testOutputMessage += L"Finished Out of Proc example\n";
}
