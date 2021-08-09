//--------------------------------------------------------------------------------------
// OutOfProcDumpTool.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include <DbgHelp.h>
#include "../SharedMemory/SharedMemory.h"
#include "OSHelpers.h"

using namespace SharedMemory;

namespace
{
#if !defined(_GAMING_XBOX)
    BOOL EnableDebugPrivilege(BOOL bEnable)
    {
        HANDLE hToken = nullptr;
        LUID luid;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) return FALSE;
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) return FALSE;

        TOKEN_PRIVILEGES tokenPriv;
        tokenPriv.PrivilegeCount = 1;
        tokenPriv.Privileges[0].Luid = luid;
        tokenPriv.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

        if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) return FALSE;

        return TRUE;
    }
#endif

    bool FileExists(const wchar_t *fileName)
    {
        uint32_t attrs = ::GetFileAttributesW(fileName);

        return (attrs != INVALID_FILE_ATTRIBUTES
            && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
    }

    wchar_t *GetDumpFileName(const wchar_t *baseName)
    {
        static wchar_t outFile[MAX_PATH];
        memset(outFile, 0, sizeof(outFile));
        {
            wcscat_s(outFile, baseName);
            wchar_t *dotLoc = outFile + wcslen(baseName);
            wcscat_s(outFile, L".dmp");

            int num = 0;
            wchar_t suffix[16] = L"";

            wchar_t *underscore = dotLoc;
            while (FileExists(outFile))
            {
                *underscore = L'\0';
                swprintf_s(suffix, L"_%i.dmp", ++num);
                wcscat_s(outFile, suffix);
            }
        }

        return outFile;
    }

    void WriteDump(HANDLE proc, DWORD procId, MINIDUMP_TYPE mdt, const wchar_t *dumpFileName)
    {
        if (!dumpFileName)
        {
            DX::ThrowIfFailed(E_INVALIDARG);
        }

        // Create the dump file
        DX::ScopedHandle dumpFile(DX::safe_handle(
            CreateFile(
                dumpFileName,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL)));

        if (!dumpFile)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        MINIDUMP_EXCEPTION_INFORMATION exceptionData;
        exceptionData.ThreadId = g_sharedMemory->threadId;
        exceptionData.ClientPointers = TRUE;
        exceptionData.ExceptionPointers = g_sharedMemory->pointers;
        if (!MiniDumpWriteDump(proc, procId, dumpFile.get(), mdt, &exceptionData, nullptr, nullptr))
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        if (!FlushFileBuffers(dumpFile.get()))
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    void WriteDumpForFileName(const wchar_t *filename, const wchar_t *baseDumpName, MINIDUMP_TYPE mdt)
    {
        int processCount = 0;
        DWORD procIds[1024];
        {
            DWORD resultSize = 0;
            if (EnumProcesses(procIds, sizeof(procIds), &resultSize))
            {
                processCount = resultSize / sizeof(DWORD);
            }
            else
            {
                DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        for (int i = 0; i < processCount; ++i)
        {
            HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, true, procIds[i]);
            if (proc)
            {
                wchar_t procName[MAX_PATH];

                size_t nameLength = GetProcessImageFileName(proc, procName, MAX_PATH);
                auto err = (nameLength == 0) ? GetLastError() : 0;

                if (!err)
                {
                    procName[MAX_PATH - 1] = 0;
                    wchar_t *fn = wcsrchr(procName, L'\\');
                    fn = fn ? fn + 1 : procName;

                    if (_wcsicmp(fn, filename) == 0)
                    {
                        WriteDump(proc, procIds[i], mdt, GetDumpFileName(baseDumpName));
                        break;
                    }
                }
                CloseHandle(proc);
            }
        }
    }
} // ANONYMOUS namespace

int wmain(int /*argc*/, wchar_t ** /*argv*/)
{
    if (!InitSharedMemory())
        return 1;

    for (;;)
    {
        WaitForSingleObject(g_sharedStartEvent, INFINITE);

        if (g_sharedMemory->shutdown)
            break;

        const wchar_t *exeName = g_sharedMemory->applicationName;
        const wchar_t *baseDumpName = g_sharedMemory->baseDumpName;

        try
        {
#if !defined(_GAMING_XBOX)
            EnableDebugPrivilege(true);
#endif
            WriteDumpForFileName(exeName, baseDumpName, (MINIDUMP_TYPE)g_sharedMemory->miniDumpType);
            SetEvent(g_sharedFinishedEvent);
        }
        catch (...)
        {
            SetEvent(g_sharedFinishedEvent);
            break;
        }
    }

    CleanupSharedMemory();

    return 0;
}
