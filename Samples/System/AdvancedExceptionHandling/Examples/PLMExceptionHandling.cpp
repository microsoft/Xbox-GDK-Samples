//--------------------------------------------------------------------------------------
// PLMExceptionHandling.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"

extern "C" LONG WINAPI PLMExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers);

namespace
{
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
            char* fred = nullptr;
            *fred = 5;
        }
        __except (PLMExceptionFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

extern "C" LONG WINAPI PLMExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    Sample::m_testOutputMessage += L"Handled Exception Handler called\n";

    // The title is currently suspending or resuming when an exception happened
    // In this case title exception handling code should not be executing or there is a possiblity of extending past the timer
    // If the work extends beyond the timer for suspend/resume the title will be forcibly terminated in the middle of handling the exception
    if (!Sample::s_isSuspending)
    {
        GenerateDump(exceptionPointers);
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

// This example code is very similar to other examples in this sample.
// The key piece to look at is the use of the Sample::s_isSuspending flag in the exception handler
// This flag signals if the title is currently in its suspend handler to avoid performing the expensive work of the exception handler
void Sample::PLMExceptionHandling()
{
    m_lastTestRun = e_plmExceptionHandling;
    m_testOutputMessage = L"Starting PLM Exception Handling example\n";

    m_lastTestRun = e_plmExceptionHandling;

    g_previousExceptionFilter = SetUnhandledExceptionFilter(PLMExceptionFilter);

    ForceException();

    SetUnhandledExceptionFilter(g_previousExceptionFilter);
    g_previousExceptionFilter = nullptr;

    m_testOutputMessage += L"Finished PLM Exception Handling example\n";
}
