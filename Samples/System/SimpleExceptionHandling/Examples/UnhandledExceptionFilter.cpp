//--------------------------------------------------------------------------------------
// UnhandledExceptionFilter.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleExceptionHandling.h"

namespace
{
    // when calling SetUnhandledExceptionFilter the previous filter is returned
    // this should be saved so that if needed it can be called after the new unhandled exception has finished executing
    // this allows the Windows Error Reporting system to process the error and upload the data to MS telemetry for processing
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionFilter = nullptr;

    // Pointer used to create an access violation
    uint32_t *g_nullPointer = nullptr;

    // used to avoid an infinite loop in this sample where it's continously trying to write a null pointer
    uint32_t g_numTimesFired = 0;
}

extern "C" LONG WINAPI HandledExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    Sample::m_testOutputMessage += L"Handled Exception Handler called\n";
    GenerateDump(exceptionPointers);
    g_numTimesFired++;
    // This particular sample knows that is only catching a null pointer reference here so this is a safe way to continue past the offending code
    // after the third attempt to continue past the exception update the instruction pointer to be after the null pointer dereference
    // this is done in this sample to avoid terminating the sample
    if (g_numTimesFired > 2)
    {
        Sample::m_testOutputMessage += L"Fixup instruction pointer to bypass null pointer dereference\n";
        exceptionPointers->ContextRecord->Rip++;
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

void Sample::ExecuteHandledException()
{
    m_testOutputMessage = L"Starting Unhandled Exception Filter Sample\n";
    m_lastTestRun = e_handled;
    g_numTimesFired = 0;

    g_previousExceptionFilter = SetUnhandledExceptionFilter(HandledExceptionFilter);

    // The Visual Studio debugger will always stop on an access violation such as a null pointer dereference
    // Running this sample without the debugger attached will cause the unhandled exception filter to be called
    // If the debugger is attached it will instead use the RaiseException function while will still stop in the debugger
    // but you can continue execution. Since the debugger caught the exception the unhandled exception filter will not be called
    if (IsDebuggerPresent())
        RaiseException(ERROR_UNIDENTIFIED_ERROR, 0, 0, nullptr);
    else
        *g_nullPointer = 5;
    SetUnhandledExceptionFilter(g_previousExceptionFilter);
    g_previousExceptionFilter = nullptr;
    m_testOutputMessage += L"Finished Unhandled Exception Filter Sample\n";
}

//
//
//  Ignoring an exception using the the Unhandled Exception Filter
//
//

LONG __stdcall IgnoredExceptionFilter(EXCEPTION_POINTERS* /*exceptionPointers*/)
{
    Sample::m_testOutputMessage += L"Ignored Exception Handler called\n";

    // attempt to ignore the exception and continue executing the main code flow
    // Note: The main code flow will continue execution at the same instruction pointer
    // This means if it was an exception such as an access violation the exception will continue to be thrown resulting in an infinite loop
    return  EXCEPTION_CONTINUE_EXECUTION;
}

void Sample::ExecuteIgnoredException()
{
    m_testOutputMessage = L"Starting Ignored Exception Filter Sample\n";
    m_lastTestRun = e_ignored;
    SetUnhandledExceptionFilter(IgnoredExceptionFilter);

    // Raising a general exception that will be caught by the Unhandled Exception filter. In this example the filter returns EXCEPTION_CONTINUE_EXECUTION.
    // The debugger can continue past these types of exceptions without issue if desired
    // However since the debugger caught the exception it is not actually an unhandled exception which means the filter will not be called
    RaiseException(ERROR_UNIDENTIFIED_ERROR, 0, 0, nullptr);

    SetUnhandledExceptionFilter(g_previousExceptionFilter);
    g_previousExceptionFilter = nullptr;
    m_testOutputMessage += L"Finished Ignored Exception Filter Sample\n";
}
