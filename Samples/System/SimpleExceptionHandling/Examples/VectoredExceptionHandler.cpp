//--------------------------------------------------------------------------------------
// VectoredExceptionHandler.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleExceptionHandling.h"

namespace
{
    uint32_t g_calledVectoredExceptionHandler[2] = { UINT32_MAX,UINT32_MAX };
}

LONG __stdcall VectoredExceptionHandlerFront(EXCEPTION_POINTERS* exceptionPointers)
{
    Sample::m_testOutputMessage += L"Called first vectored exception handler\n";
    GenerateDump(exceptionPointers);

    if (g_calledVectoredExceptionHandler[0] == UINT32_MAX)
        g_calledVectoredExceptionHandler[0] = 1;
    else
        g_calledVectoredExceptionHandler[1] = 1;

    // continue on to the next vectored exception handler
    // Possible return values from the vectored exception filter
    // EXCEPTION_EXECUTE_HANDLER - directly proceed the top level exception handler in the system, passes control to MS crash reporting telemetry and terminates the title
    // EXCEPTION_CONTINUE_SEARCH - Proceed to the next vectored exception filter in the chain, or the unhandled exception filter if at the end of the chain.
    // EXCEPTION_CONTINUE_EXECUTION - Continue execution at the point of the exception
    return  EXCEPTION_CONTINUE_SEARCH;
}

LONG __stdcall VectoredExceptionHandlerBack(EXCEPTION_POINTERS* /*exceptionPointers*/)
{
    Sample::m_testOutputMessage += L"Called second vectored exception handler\n";
    if (g_calledVectoredExceptionHandler[0] == UINT32_MAX)
        g_calledVectoredExceptionHandler[0] = 2;
    else
        g_calledVectoredExceptionHandler[1] = 2;

    // break out of the vectored exception handler chain and continue execution
    // we know this is the last handler in the chain, if EXCEPTION_CONTINUE_SEARCH was called it would default to the unhandled exception filter
    // Possible return values from the vectored exception filter
    // EXCEPTION_EXECUTE_HANDLER - directly proceed the top level exception handler in the system, passes control to MS crash reporting telemetry and terminates the title
    // EXCEPTION_CONTINUE_SEARCH - Proceed to the next vectored exception filter in the chain, or the unhandled exception filter if at the end of the chain.
    // EXCEPTION_CONTINUE_EXECUTION - Continue execution at the point of the exception
    return  EXCEPTION_CONTINUE_EXECUTION;
}

void Sample::ExecuteVectoredException()
{
    m_testOutputMessage = L"Starting Vectored Exception Filter Sample\n";
    m_lastTestRun = e_vectored;
    g_calledVectoredExceptionHandler[0] = UINT32_MAX;
    g_calledVectoredExceptionHandler[1] = UINT32_MAX;

    // add the function VectoredExceptionHandlerFront to the front of the list of Vectored Exception filters
    void *handler1 = AddVectoredExceptionHandler(1, VectoredExceptionHandlerFront);

    // add the function VectoredExceptionHandlerBack to the end of the list of Vectored Exception filters
    void *handler2 = AddVectoredExceptionHandler(0, VectoredExceptionHandlerBack);

    // Raise an exception to be caught by the Vectored Exception system
    // Since the vectored exception handlers said used EXCEPTION_CONTINUE_EXECUTION at the end this exception does not become and unhandled exception
    // Since it's not an unhandled exception the debugger will not stop here
    RaiseException(ERROR_UNIDENTIFIED_ERROR, 0, 0, nullptr);

    RemoveVectoredExceptionHandler(handler1);
    RemoveVectoredExceptionHandler(handler2);

    m_testOutputMessage += L"Finished Vectored Exception Filter Sample\n";
}
