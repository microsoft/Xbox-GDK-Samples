//--------------------------------------------------------------------------------------
// StructuredExceptions.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleExceptionHandling.h"

void Sample::ExecuteStructuredException()
{
    m_lastTestRun = e_structured;
    m_testOutputMessage = L"Starting Structured Exception Handling (SEH) Sample\n";

    // Structured exception handled covers exceptions that happen within the context of a __try block
    // If an exception occurs the code looks at the parameter to the next __except block
    // If the parameter evaluates EXCEPTION_EXECUTE_HANDLER the code within the handler is called
    __try
    {
        uint32_t *nullPointer = nullptr;
        *nullPointer = 5;
    }

    // when the null pointer is dereferenced the exception system will determine what to do based on the parameter to __except
    // In this case it will generate a crash dump using the exception information and then evaluate to execute the code in the block
    // calling the GetExceptionInformation function is only valid within the __except parameter processing
    // Parameters for __except, these match the return value from an unhandled exception filter
    // EXCEPTION_EXECUTE_HANDLER - Stop at this handler and execute block of code attached to it
    // EXCEPTION_CONTINUE_SEARCH - Skip this __except block and continue up the callstack to find the next one
    // EXCEPTION_CONTINUE_EXECUTION - Continue execution at the point of the exception skipping both the attached code and searching for another __except block
    __except (GenerateDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        m_testOutputMessage += L"In the Structured Exception Handler\n";
    }

    m_testOutputMessage += L"Finished Structured Exception Handling (SEH) Sample\n";
}
