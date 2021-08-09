//--------------------------------------------------------------------------------------
// Recommended.cpp
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
}

void ThreadWorkingRecommendedFunction();
void ThreadEntryRecommendedFunction();

// Entry point for any title created thread should be protected with a structured exception handler
// This will allow each thread to capture the exception for processing before the unhandled exception filter is called
// If the title doesn't care about capturing exceptions at the thread level it's safe to remove this structured exception wrapper
void ThreadEntryRecommendedFunction()
{
    Sample::m_testOutputMessage += L"Starting Thread Entry Recommended Function\n";
    __try
    {
        // The title can do any code it wants here, it does not have to place all its main code in a second function
        // The part to remember is that you can't have a structured exception handler in a function that requires stack unwinding.
        ThreadWorkingRecommendedFunction();
    }

    // It's important to return EXCEPTION_CONTINUE_SEARCH from the __except block if the title cannot continue execution after the exception
    // the reason is to allow the unhandled exception filter to be called and eventually call the Microsoft exception handler
    // This allows the Microsoft crash telemetry system to collect the data and upload it to the Microsoft servers for processing
    // Titles are given access to this data for their own use if desired
    __except (GenerateDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {                                                   // This should be EXCEPTION_CONTINUE_SEARCH to allow MS telemetry to collect crash data
        Sample::m_testOutputMessage += L"In the Thread Exception __except block\n";
    }

    Sample::m_testOutputMessage += L"Finished Thread Entry Recommended Function\n";
}

// the next function to call from the Thread entry point, this would be where the main work would be performed
// See the notes on the Thread Entry function above for the reason to split these two functions
void ThreadWorkingRecommendedFunction()
{
    Sample::m_testOutputMessage += L"Starting Thread Working Recommended Function\n";

    // the need for this C++ try/catch block is dependent on the title and its usage of C++ exception handling
    // Still it's advisable to have a block like this to catch possible unknown C++ exceptions that might be an error for your title
    // Note: There is no runtime overhead for C++ exception handling in the x64 environment
    // The overhead associated with C++ exception handling happens at exception time. There is a static function frame table that
    // is stored with your title. This is used for frame unwinding, including possible stack unwinding by the title
    try
    {
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
    Sample::m_testOutputMessage += L"Finished Thread Working Recommended Function\n";
}

extern "C" LONG WINAPI GlobalExceptionFilter(LPEXCEPTION_POINTERS const exceptionPointers)
{
    Sample::m_testOutputMessage += L"Global Exception Handler called\n";
    GenerateDump(exceptionPointers);

    // The recommended action is to call the previous unhandled exception filter, returned by the call to SetUnhandledExceptionFilter
    // In this case Windows Error Reporting would be called to save a crash dump, upload it to the Microsoft Servers, and terminate the title
    return g_previousExceptionFilter(exceptionPointers);
}

void Sample::ExecuteRecommended()
{
    m_lastTestRun = e_recommended;
    m_testOutputMessage = L"Starting Recommended Exception Handling Sample\n";

    // Call this as early as possible during title startup, the title won't catch exceptions before this function is called
    // This allows for the Microsoft telemtry to catch any exceptions that might be generated during title cleanup by the OS.
    g_previousExceptionFilter = SetUnhandledExceptionFilter(GlobalExceptionFilter);

    // Continue title initialization and execution
    ThreadEntryRecommendedFunction();

    // When the Unhandled Exception Filter is no longer needed by the title the filter should be restored to the previous value
    SetUnhandledExceptionFilter(g_previousExceptionFilter);
    g_previousExceptionFilter = nullptr;

    m_testOutputMessage += L"Finished Recommended Exception Handling Sample\n";
}
