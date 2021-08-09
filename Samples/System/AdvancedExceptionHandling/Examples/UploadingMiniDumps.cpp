//--------------------------------------------------------------------------------------
// UploadingMiniDumps.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AdvancedExceptionHandling.h"

// The WER exception system built into the console saves crash dumps to the disk for later upload when the kit is idle
// Titles should follow this model and should upload crash dumps when their title is idle and stable. A good time is at the next execution of the title
// Attempting to upload a crash dump during the handling of an exception can cause many issues and the system may be in an unstable state
// For example the network layer could be the source of the exception, trying to upload a crash dump during this time could easily result in failure

namespace
{
    std::vector<std::wstring> g_foundCrashDumps;
    void UploadCrashDumpThread()
    {
        for (auto& iter : g_foundCrashDumps)
        {
            Sample::m_testOutputMessage += L"Found and uploaded dump file: ";
            Sample::m_testOutputMessage += iter;
            Sample::m_testOutputMessage += L"\n";

            // Upload the file, when the upload is complete delete the file.
            // If something happens where an upload is interrupted the original file is still on the disk and the next attempt will try again.
            DeleteFileW(iter.c_str());
        }
    }
}

void Sample::UploadingMiniDumps()
{
    std::wstring basePath;
    m_lastTestRun = e_uploadingMiniDumps;
    m_testOutputMessage = L"Starting Uploading Crash Dumps example\n";

#ifdef _GAMING_DESKTOP
    {
        wchar_t currentDirectory[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, currentDirectory);
        basePath = currentDirectory;
        basePath += L"\\";
    }
#else
    basePath = L"d:\\";
#endif

    // Scan the drive finding all the current crash dump files and add them to the upload list
    WIN32_FIND_DATAW findData;
    std::wstring searchPath(basePath);
    searchPath += L"*.dmp";
    HANDLE searchHandle = FindFirstFileW(searchPath.c_str(), &findData);
    if (searchHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring foundName = findData.cFileName;
            if (findData.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY)
            {
                std::wstring fullName = basePath;
                fullName += foundName;

                g_foundCrashDumps.push_back(fullName);

                continue;
            }
        } while (FindNextFileW(searchHandle, &findData));
        FindClose(searchHandle);
    }

    if (g_foundCrashDumps.size() != 0)
    {
        // Once all the crash dumps have been found startup a background thread to process them while the title can continue running
        std::thread *uploadThread = new std::thread(UploadCrashDumpThread);

        // continue running normal game code

        uploadThread->join();
        delete uploadThread;
    }
    else
    {
        m_testOutputMessage += L"Did not find any crash dumps for upload\n";
    }

    m_testOutputMessage += L"Finished Uploading Crash Dumps example\n";
}
