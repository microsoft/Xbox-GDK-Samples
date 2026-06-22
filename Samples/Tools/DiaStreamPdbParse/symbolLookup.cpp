//--------------------------------------------------------------------------------------
// symbolLookup.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "symbolLookup.h"

#include "dia2.h"
#include "diacreate.h"

#include <wrl\implements.h>
#include "PdbMemoryStream.h"
#include "CallStack.h"

#ifdef _GAMING_DESKTOP
#include <psapi.h>
#endif

// Looking up all the inlined functions for heavily inlined code can become expensive
#define DEFAULT_RETURN_INLINED_FUNCTIONS true

// the source file and line numbers may not be needed for the callstack so don't waste time looking them up
#define DEFAULT_RETURN_SOURCE_LINENUMBERS true

// Getting the undecorated name for a function is more expensive than getting the base name
#define DEFAULT_USE_UNDECORATED_NAMES true

using namespace Microsoft::WRL;

#define RETERR(__exp) { \
    HRESULT __hr = (__exp); \
    if (FAILED(__hr) || __hr == S_FALSE) { \
        return __hr; \
    } \
}

namespace
{
    static const uint32_t c_maxCapturedFrames = 128;
    static const uint64_t c_pdbNumCacheBlocks = 32;
    static const uint64_t c_pdbCacheBlockSize = 32768;

    // Represents a single PDB file that has been opened using DIA
    struct PDBSession
    {
        ComPtr<IDiaDataSource> source{};        // High level interface for performing queries from a PDB file, one-to-one mapping of instance to PDB file
        ComPtr<IPdbMemoryStream> pdbStream{};   // The IStream interface that is responsible for reading the PDB from disk
        ComPtr<IStream> diaStream{};            // The same object as pdbStream, cached to make some of the later code cleaner
        ComPtr<IDiaSession> session{};          // A single query context for an IDiaDataSource, there can be multiple sessions open if desired
        std::wstring fullModuleName;            // The full path to the exe/dll file
        std::wstring shortModuleName;           // The name of the exe/dll file
        std::wstring pdbName;                   // The full path to the PDB file
        uintptr_t moduleBase{};                  // This code is using the RVA (relative virtual address) lookups within DIA. This is the offset from the base address the exe/dll was loaded
    };

    // DIA returns BSTR objects for things like source file and function names. Normally within Windows these are managed with SysAllocString and SysFreeString
    // DIA allocates the BSTR object with SysAllocString, the caller would then free the BSTR with SysFreeString.
    // However on the Xbox SysAllocString/SysFreeString is not available, the provided version of msdia140-xbox.dll instead uses HeapAlloc.
    // Since the Xbox version uses HeapAlloc the title can free the string with HeapFree.
    // BSTR objects have a 32-bit length at the start of the string so the pointer needs to be moved back 4-bytes before calling HeapFree
    class DIAString
    {
    public:
        DIAString() : m_realString(nullptr) { }
        DIAString(const DIAString& rhs) = delete;
        DIAString(DIAString&& rhs) = delete;
#ifdef _GAMING_DESKTOP
        // The desktop environment does suppport SysAllocString/SysFreeString so use that method for cleanup. This also allows the Desktop version to use the default msdia140.dll
        ~DIAString() { if (m_realString) 	SysFreeString(m_realString); }
#else
        // On Xbox subtract four bytes from the pointer and call HeapFree to release the memory
        ~DIAString() { if (m_realString) 	HeapFree(GetProcessHeap(), 0, reinterpret_cast<BYTE*>(m_realString) - sizeof(DWORD)); }
#endif
        operator wchar_t* () const { return m_realString; }
        operator BSTR* () { return &m_realString; }
        operator bool() { return m_realString != nullptr; }

    private:
        BSTR m_realString;
    };
}

// Starting with an RVA and an optional rootSymbol locate the corresponding source file and line number
// The optional rootSymbol is used to find inlined function information, this is the matching inlined frame from the parent non-inlined function
void LookupSourceLineNumber(uint32_t rva, IDiaSymbol *rootSymbol, IDiaSession* session, std::vector<StackEntry>& output)
{
    ComPtr<IDiaEnumLineNumbers> lines{};
    HRESULT findHR = S_FALSE;

    // PDB files include information for inlined functions so a specific RVA can be mapped back to the source from an inlined function
    // In this case it works off the parent symbol, which would be the outer non-inlined function
    if (rootSymbol)
        findHR = rootSymbol->findInlineeLinesByRVA(rva, 1, &lines);
    else if (session)
        findHR = session->findLinesByRVA(rva, 1, &lines);

    if (findHR != S_OK)
        return;
    ComPtr<IDiaLineNumber> tmpLine;
    DWORD celt;
    DWORD previousFileId = 0;

    // The second parameter to the previous findLines... functions indicate the length of the memory block for the query
    // This code only askes for a one byte range which should map back to a single line
    // However we're going to iterate over all the returned lines for completeness
    while (lines->Next(1, &tmpLine, &celt) == S_OK && celt == 1)
    {
        // Iterating through an IDiaEnumLineNumber object returns IDiaLineNumber objects
        ComPtr<IDiaLineNumber> line(std::move(tmpLine));
        DWORD fileId = 0;
        do
        {
            // Given the IDiaLineNumber entry you need to query for the file ID to get information about that file
            if (line->get_sourceFileId(&fileId) != S_OK)
                break;

            DIAString sourceFileName;
            if (fileId != previousFileId)
            {
                // Use the file ID to query for the file block to finally get the name of the source file
                ComPtr<IDiaSourceFile> source;
                if (line->get_sourceFile(&source) == S_OK)
                {
                    if (FAILED(source->get_fileName(sourceFileName)))
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            // Using the same IDiaLineNumber entry query it for the actual line number, then build the returned source file and line number string
            DWORD lineStart;
            if (line->get_lineNumber(&lineStart) == S_OK)
            {
                output.push_back(StackEntry(L"", sourceFileName ? (wchar_t*)sourceFileName : L"Unknown Source File", lineStart));
            }
        } while (FALSE);
        previousFileId = fileId;
    }
}

// Given a set RVA fill the output buffer with all of the matching source file and line numbers that match
// When inline functions are involved it's possible for an RVA to map to multiple source/line number combinations
// When looking up symbols in a session using an RVA you will get the parent non-inlined function
HRESULT DumpRvaToSourceLine(uint32_t rva, std::vector<StackEntry>& output, PDBSession& session, bool includeInline = DEFAULT_RETURN_INLINED_FUNCTIONS, bool includeSourceFile = DEFAULT_RETURN_SOURCE_LINENUMBERS, bool undecoratedNames = DEFAULT_USE_UNDECORATED_NAMES)
{
    ComPtr<IDiaSymbol> rootSymbol{};
    LONG lDisplacement;
    HRESULT findHR;

    // Find the root symbol for this RVA, in this case looking for the closest function to the RVA
    // With inlined functions this will be the parent non-inlined function
    findHR = session.session->findSymbolByRVAEx(rva, SymTagFunction, &rootSymbol, &lDisplacement);
    if (!rootSymbol)
        return findHR;

    DIAString name;
    if (undecoratedNames)
    {
        findHR = rootSymbol->get_undecoratedName(name);
    }
    else
    {
        findHR = rootSymbol->get_name(name);
    }

    // Looking up inlined functions can be expensive when you consider how many minor functions could exist
    // because of this we give the caller the option to not lookup any inline functions
    if (includeInline)
    {
        // look for matching inlined functions first so the final stack output looks correct for function call order
        ComPtr<IDiaEnumSymbols> inlineFunctionEnum;
        findHR = rootSymbol->findInlineFramesByRVA(rva, &inlineFunctionEnum);
        if (inlineFunctionEnum)
        {
            uint32_t symCount;
            findHR = inlineFunctionEnum->get_Count((LONG *)&symCount);
            // the order in the list matches the call order of the inlined functions
            for (uint32_t i = 0; i < symCount; i++)
            {
                DIAString inlineFunctionName;
                ComPtr<IDiaSymbol> inlineFrame;
                findHR = inlineFunctionEnum->Item(i, &inlineFrame);
                findHR = inlineFrame->get_name(inlineFunctionName);
                // Getting the information for the source file and line number is not always needed, not querying the data removes 50% of the overhead
                if (includeSourceFile)
                {
                    std::vector<StackEntry> inlineNames;
                    LookupSourceLineNumber(rva, inlineFrame.Get(), nullptr, inlineNames);
                    for (const auto& iter : inlineNames)
                    {
                        output.push_back(StackEntry((wchar_t *)inlineFunctionName, iter.sourceFileName, iter.lineNumber, StackEntry::Flag::InlineFrame));
                    }
                }
                else
                {
                    output.push_back(StackEntry((wchar_t *)inlineFunctionName, L"", 0, StackEntry::Flag::InlineFrame));
                }
            }
        }
    }

    // Getting the information for the source file and line number is not always needed, not querying the data removes 50% of the overhead
    if (includeSourceFile)
    {
        // After all the inlined frames are processed append the function name, source file, and line number to the output
        std::vector<StackEntry> rootSymbolNames;
        LookupSourceLineNumber(rva, nullptr, session.session.Get(), rootSymbolNames);
        for (const auto& iter : rootSymbolNames)
        {
            output.push_back(StackEntry((wchar_t *)name, iter.sourceFileName, iter.lineNumber));
        }
    }
    else
    {
        output.push_back(StackEntry((wchar_t *)name));
    }

    return S_OK;
}

// A IDiaDataSource object needs to be created for each PDB being processed
// This is done by referencing the DIA DLL directly using NoRegCoCreate, this is done to bypass the need to have the DIA COM object registered
HRESULT LoadDIA(PCWSTR diaPath, PDBSession& sessionDetails)
{
    if (sessionDetails.source != nullptr)
        return S_OK;

#ifdef _GAMING_DESKTOP
    ((void)diaPath);
    return CoCreateInstance(__uuidof(DiaSource),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IDiaDataSource),
        (void **)&sessionDetails.source);
#else
    // Initial check to make sure the DIA DLL is available for loading
    WIN32_FILE_ATTRIBUTE_DATA fileData{};
    if (!GetFileAttributesExW(diaPath, GetFileExInfoStandard, &fileData))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Create source not using registry
    return NoRegCoCreate(diaPath,
        CLSID_DiaSource,
        __uuidof(IDiaDataSource),
        (void**)&sessionDetails.source);
#endif
}

// Once an IDiaDataSource is created through LoadDIA an IDiaSession needs to be created binding it to the PDB and the PdbMemoryStream
HRESULT LoadPDB(PDBSession& sessionDetails, uint64_t numCacheBlocks, uint64_t cacheBlockSize)
{
    if (sessionDetails.source == nullptr)
        return E_UNEXPECTED;

    // Initial check to make sure the PDB file is available
    WIN32_FILE_ATTRIBUTE_DATA fileData{};
    if (!GetFileAttributesExW(sessionDetails.pdbName.c_str(), GetFileExInfoStandard, &fileData))
    {
        // Since we're on desktop attempt to fallback to the normal PDB API path, this will bypass the PdbMemoryStream interface
        // This requires the PDB to be locatable through either a symbol server or locally.
        // See https://docs.microsoft.com/visualstudio/debugger/debug-interface-access/idiadatasource-loaddataforexe for more details
#ifdef _GAMING_DESKTOP
        RETERR(sessionDetails.source->loadDataForExe(sessionDetails.fullModuleName.c_str(), nullptr, nullptr));
        RETERR(sessionDetails.source->openSession(&sessionDetails.session));
        RETERR(sessionDetails.session->put_loadAddress(0));
#endif
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Create an InMemoryPdbStream instance that is bound to the PDB file
    // The IDiaDataSource instance will use this object to read data from the PDB, it will not access the file directly
    RETERR(CreatePdbMemoryStream(sessionDetails.pdbName.c_str(), numCacheBlocks, cacheBlockSize, IID_PPV_ARGS(&sessionDetails.pdbStream)));

    // Bind the InMemoryPdbStream instance to the IDiaDataSource instance
    RETERR(sessionDetails.pdbStream->QueryInterface(IID_PPV_ARGS(&sessionDetails.diaStream)));
    RETERR(sessionDetails.source->loadDataFromIStream(sessionDetails.diaStream.Get()));

    // At this point the IDiaSession can be created, this call will immediately read at least the header data from the PDB using the InMemoryPdbStream instance
    RETERR(sessionDetails.source->openSession(&sessionDetails.session));

    // It's possible to use both RVA (relative virtual addresses) and absolute addresses with DIA
    // The queries that take an absolute address subtract this load address from them to generate the RVA used internally
    // In this case just making sure the load address is set to 0 for this IDiaSession instance
    RETERR(sessionDetails.session->put_loadAddress(0));

    return S_OK;
}

// Root entry point for the symbol lookup code. It takes either a thread or an EXCEPTION_POINTERS block and returns the matching callstack
// The exceptionPointers parameter can come straight from either unhandled exception filter or a structured exception handler
// The whichThread parameter allows an easy check to see where a thread is currently executing, if the thread is executed it is suspended to capture it's callstack
// If neither whichThread or exceptionPointers is specified a callstack is returned for the current thread
// The exceptionPointers parameter takes precedance over the whichThread parameter
HRESULT DumpCallstack(std::vector<std::wstring>& output, HANDLE whichThread, EXCEPTION_POINTERS* exceptionPointers)
{
    output.clear();
    std::vector<StackEntry> stackDetails;
    void* capturedAddresses[c_maxCapturedFrames];
    HMODULE capturedModules[c_maxCapturedFrames];
    std::map<HMODULE, PDBSession> modules;
    uint32_t rvas[c_maxCapturedFrames];
    size_t capturedFrames = 0;
    if (exceptionPointers)
    {
        // The context record for the faulting thread is contained in the EXCEPTON_POINTERS block
        capturedFrames = CallStack::CaptureBackTraceFromContext(exceptionPointers->ContextRecord, &capturedAddresses[0], capturedModules, c_maxCapturedFrames);
    }
    else if (whichThread == INVALID_HANDLE_VALUE)
    {
        capturedFrames = CallStack::CaptureBackTraceFromCurrentThread(&capturedAddresses[0], capturedModules, c_maxCapturedFrames);
    }
    else
    {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;
        // The thread context data is only updated on a context switch, we also don't want the thread running while its callstack is being walked
        // Because of these conditions we suspend the thread, this will update the context record as well as keeping it from running while the stack is walked
        // Once the call stack is captured the thread can continue to execute and is resumed
        SuspendThread(whichThread);
        if (GetThreadContext(whichThread, &ctx))
        {
            capturedFrames = CallStack::CaptureBackTraceFromContext(&ctx, &capturedAddresses[0], capturedModules, c_maxCapturedFrames);
        }
        ResumeThread(whichThread);
    }

    for (size_t i = 0; i < capturedFrames; i++)
    {
        // The CaptureBackTrace functions return an array of absolute addresses and the module they map into
        // The RVA is just the absolute address - the module address. The module address is just the HMODULE returned by GetModuleHandle
        rvas[i] = static_cast<uint32_t> (reinterpret_cast<uint64_t>(capturedAddresses[i]) - reinterpret_cast<uint64_t>(capturedModules[i]));

        // If the module hasn't already been seen add an entry into the unique module map with it's base address, full path, and short name
        if (modules.find(capturedModules[i]) == modules.end())
        {
            wchar_t fileName[MAX_PATH];
            if (GetModuleFileNameW(capturedModules[i], fileName, MAX_PATH))
            {
                modules[capturedModules[i]].fullModuleName = fileName;
                modules[capturedModules[i]].moduleBase = (uintptr_t)capturedModules[i];

                auto slashPos = modules[capturedModules[i]].fullModuleName.rfind(L'\\');
                if (slashPos == std::wstring::npos)
                    continue;
                modules[capturedModules[i]].shortModuleName = modules[capturedModules[i]].fullModuleName.substr(slashPos + 1);
            }
        }
    }

    const wchar_t* diaPath = nullptr;
#ifdef _GAMING_XBOX
    // Load the xbox version of msdia140, this is the version that ships with VS2022 but memory allocations have been converted to HeapAlloc
    diaPath = L"DIA_XBOX\\msdia140-xbox.dll";
#endif

    HRESULT hr;
    // For each unique module in the callstack attempt to create an IDiaSession with the matching PDB
    for (auto& iter : modules)
    {
        hr = LoadDIA(diaPath, iter.second);
        if (hr == S_OK)
        {
            auto extensionStart = iter.second.fullModuleName.rfind(L'.');
            if (extensionStart == std::wstring::npos)
                continue;
            iter.second.pdbName = iter.second.fullModuleName.substr(0, extensionStart);
            iter.second.pdbName += L".pdb";
            LoadPDB(iter.second, c_pdbNumCacheBlocks, c_pdbCacheBlockSize);
        }
    }

    // For each captured frame resolve the function name, source file, and line number using the IDiaSession matching the module containing the frame
    for (size_t i = 0; i < capturedFrames; i++)
    {
        auto moduleData = modules.find(capturedModules[i]);
        if (moduleData != modules.end())
        {
            if (moduleData->second.pdbStream == nullptr)
            {
                stackDetails.push_back(StackEntry(moduleData->second.fullModuleName, rvas[i], StackEntry::Flag::NoPDB));
            }
            else
            {
                std::vector<StackEntry> newEntries;
                hr = DumpRvaToSourceLine(rvas[i], newEntries, moduleData->second);
                if (hr != S_OK)
                {
                    stackDetails.push_back(StackEntry(moduleData->second.fullModuleName, rvas[i], StackEntry::Flag::DumpRVAFailed));
                }
                else
                {
                    for (auto& iter : newEntries)
                    {
                        iter.moduleName = moduleData->second.shortModuleName;
                        iter.rva = rvas[i];
                        stackDetails.push_back(iter);
                    }
                }
            }
        }
        else
        {
            stackDetails.push_back(StackEntry(L"", rvas[i], StackEntry::Flag::FunctionNotFound));
        }
    }

    for (const auto& iter : stackDetails)
    {
        wchar_t buffer[1024];

        switch (iter.flag)
        {
        case StackEntry::Flag::Default:
        case StackEntry::Flag::InlineFrame:
            if (iter.sourceFileName.size())
            {
                swprintf(buffer, 1024, L"%s!%s%ws\n\n", iter.moduleName.c_str(), iter.flag == StackEntry::Flag::InlineFrame ? L"[Inline Frame] " : L"", iter.functionName.c_str());
                output.push_back(buffer);
                swprintf(buffer, 1024, L"     %ws(%d)\n", iter.sourceFileName.c_str(), iter.lineNumber);
                output.push_back(buffer);
            }
            else
            {
                swprintf(buffer, 1024, L"%s!%s%ws\n", iter.moduleName.c_str(), iter.flag == StackEntry::Flag::InlineFrame ? L"[Inline Frame] " : L"", iter.functionName.c_str());
            }
            break;
        case StackEntry::Flag::NoPDB:
            swprintf(buffer, 1024, L"No PDB found %s : 0x%x\n", iter.moduleName.c_str(), iter.rva);
            output.push_back(buffer);
            break;
        case StackEntry::Flag::DumpRVAFailed:
            swprintf(buffer, 1024, L"DumpRvaToSourceLine failed %s : 0x%x\n", iter.moduleName.c_str(), iter.rva);
            output.push_back(buffer);
            break;
        case StackEntry::Flag::FunctionNotFound:
            swprintf(buffer, 1024, L"Function not found : 0x%x\n", iter.rva);
            output.push_back(buffer);
            break;
        }
    }
    return S_OK;
}
