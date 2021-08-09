//--------------------------------------------------------------------------------------
// LanguageTryCatch.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleExceptionHandling.h"

// C++ exceptions in the Standard Library are derived from std::exception defined in the <exception> header.
//
// <stdexcept> defines a number of common C++ exceptions such as std::out_of_range.
//
// The std::bad_alloc exception is defined in the <new> header.
//
// <system_error> defines std::system_error which is the C++11 method for OS level errors like GetLastError results.
//
// Note that throw std::exception("string"); is a Microsoft library extension and may not be portable to other platforms.
// Ctors that take a string for derived classes like throw::out_of_range("string"); are Standard C++.
//
// See https://cppreference.com/ for more information.

void Sample::ExecuteLanguageException()
{
    m_lastTestRun = e_language;
    m_testOutputMessage = L"Starting C++ exception handling sample\n";
    try
    {
        throw std::out_of_range("Range error exception");
    }
    catch (const std::out_of_range& /*exception*/)
    {
        m_testOutputMessage += L"Caught a C++ logic_error::out_of_range exception\n";
    }

    try
    {
        throw std::bad_alloc();
    }
    catch (const std::exception& exception)
    {
        if (dynamic_cast<const std::bad_alloc*>(&exception) != nullptr)
            m_testOutputMessage += L"Caught a C++ bad_alloc exception\n";
        else
            m_testOutputMessage += L"Caught a general C++ exception\n";
    }

    try
    {
        throw 5;
    }
    catch (...)
    {
        m_testOutputMessage += L"Default catch everything C++ catch block\n";
    }

    // It's not possible to mix Structured Exception Handling (SEH) and C++ exceptions in the same function
    // It's also not possible to use structured exceptions inside a function that needs stack unwinding
    // For these reasons using Structured Exception Handling (SEH) to catch C++ exceptions is pulled out into its own function
    CatchLanguageExceptionWithStructured();
    m_testOutputMessage += L"Finished C++ exception handling sample\n";
}

// It's not possible to mix Structured Exception Handling (SEH) and C++ exceptions in the same function
// It's also not possible to use structured exceptions inside a function that needs stack unwinding
// For these reason this particular example is pulled out into its own function
void Sample::CatchLanguageExceptionWithStructured()
{
    __try
    {
        throw std::bad_alloc();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        m_testOutputMessage += L"Structured Exception Handling (SEH) can also catch C++ exceptions, but exception data is lost\n";
    }
}
