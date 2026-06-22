//--------------------------------------------------------------------------------------
// symbolLookup.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct StackEntry
{
    enum class Flag
    {
        Default,
        InlineFrame,
        NoPDB,
        DumpRVAFailed,
        FunctionNotFound,
    };

    std::wstring moduleName;
    std::wstring functionName;
    std::wstring sourceFileName;
    uint32_t rva = 0;
    uint32_t lineNumber;
    Flag flag;

    StackEntry(const std::wstring& newFunctionName, const std::wstring& newSourceFileName = L"", uint32_t newLineNumber = 0, Flag newFlag = Flag::Default)
        :functionName(newFunctionName)
        , sourceFileName(newSourceFileName)
        , lineNumber(newLineNumber)
        , flag(newFlag)
    {}

    StackEntry(const std::wstring& newModuleName, uint32_t newRVA, Flag newFlag)
        :moduleName(newModuleName)
        , rva(newRVA)
        , lineNumber(0)
        , flag(newFlag)
    {}
};

HRESULT DumpCallstack(std::vector<std::wstring>& output, HANDLE whichThread = INVALID_HANDLE_VALUE, EXCEPTION_POINTERS* pExceptionPointers = nullptr);
