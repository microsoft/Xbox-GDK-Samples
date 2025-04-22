//--------------------------------------------------------------------------------------
// File: FindMedia.h
//
// Helper function to find the location of a media file for Windows desktop apps
// since they lack appx packaging support.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <tuple>


namespace DX
{
    inline void FindMediaFile(
        _Out_writes_(cchDest) wchar_t* strDestPath,
        _In_ int cchDest,
        _In_z_ const wchar_t* strFilename,
        _In_opt_ const wchar_t* const * searchFolders = nullptr)
    {
        if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
            throw std::invalid_argument("FindMediaFile");

        // Check CWD for quick out
        wcscpy_s(strDestPath, size_t(cchDest), strFilename);
        if (GetFileAttributesW(strDestPath) != INVALID_FILE_ATTRIBUTES)
            return;

        // Search CWD with folder names
        static const wchar_t* s_defSearchFolders[] =
        {
            L"Assets",
            L"Assets\\Fonts",
            L"Assets\\Textures",
            L"Media",
            L"Media\\Textures",
            L"Media\\Fonts",
            L"Media\\Meshes",
            L"Media\\PBR",
            L"Media\\CubeMaps",
            L"Media\\HDR",
            L"Media\\Sounds",
            L"Media\\Videos",
            0
        };

        if (!searchFolders)
            searchFolders = s_defSearchFolders;

        wchar_t strFullFileName[MAX_PATH] = {};
        for (const wchar_t* const * searchFolder = searchFolders; *searchFolder != 0; ++searchFolder)
        {
            swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", *searchFolder, strFilename);
            if (GetFileAttributesW(strFullFileName) != INVALID_FILE_ATTRIBUTES)
            {
                wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
                return;
            }
        }

        // Get the exe name, and exe path
        wchar_t strExePath[MAX_PATH] = {};
        std::ignore = GetModuleFileNameW(nullptr, strExePath, MAX_PATH);
        strExePath[MAX_PATH - 1] = 0;

        // Search all parent directories starting at .\ and using strFilename as the leaf name
        wchar_t strLeafName[MAX_PATH] = {};
        wcscpy_s(strLeafName, MAX_PATH, strFilename);

        wchar_t strFullPath[MAX_PATH] = {};
        wchar_t strSearch[MAX_PATH] = {};
        wchar_t* strFilePart = nullptr;

        std::ignore = GetFullPathNameW(strExePath, MAX_PATH, strFullPath, &strFilePart);

        while (strFilePart && *strFilePart != '\0')
        {
            swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", strFullPath, strLeafName);
            if (GetFileAttributesW(strFullFileName) != INVALID_FILE_ATTRIBUTES)
            {
                wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
                return;
            }

            for (const wchar_t* const * searchFolder = searchFolders; *searchFolder != 0; ++searchFolder)
            {
                swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls\\%ls", strFullPath, *searchFolder, strLeafName);
                if (GetFileAttributesW(strFullFileName) != INVALID_FILE_ATTRIBUTES)
                {
                    wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
                    return;
                }
            }

            swprintf_s(strSearch, MAX_PATH, L"%ls\\..", strFullPath);
            std::ignore = GetFullPathNameW(strSearch, MAX_PATH, strFullPath, &strFilePart);
        }

        // On failure, return the file as the path but also throw an error
        wcscpy_s(strDestPath, size_t(cchDest), strFilename);

#ifdef _DEBUG
        wchar_t errorMessage[1024] = {};
        swprintf_s(errorMessage, 1024, L"ERROR: FindMedia file not found: %ls\n", strFilename);
        OutputDebugStringW(errorMessage);
#endif

        throw std::runtime_error("File not found");
    }
}
