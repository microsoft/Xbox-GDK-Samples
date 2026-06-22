//--------------------------------------------------------------------------------------
// TextureFiles.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "TextureFiles.h"
#include "ToolDefines.h"
#include "util.hpp"

UINT32 g_SceneTextureCount = 0;
WCHAR g_strTextureBasePath[MAX_PATH] = L"d:\\streamingtextures\\";
const WCHAR g_strTextureBasePath2[MAX_PATH] = L"streamingtextures\\";

bool FileExists(const WCHAR* strFileName)
{
    HANDLE hFile = CreateFile2(strFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        HRESULT hr = GetLastError();
        DebugSpew("FileExists error 0x%08x\n", static_cast<unsigned int>(hr));
        return false;
    }
    CloseHandle(hFile);
    return true;
}

void ScanForTextures()
{
    UINT32 TextureCount = 0;
    bool Found = true;

    while (Found)
    {
        WCHAR strDiffuseFileName[MAX_PATH];
        WCHAR strNormalFileName[MAX_PATH];
        WCHAR strSpecularFileName[MAX_PATH];
        CreateTextureFilename(strDiffuseFileName, ARRAYSIZE(strDiffuseFileName), TextureCount, TextureSuffix_Diffuse, false);
        CreateTextureFilename(strNormalFileName, ARRAYSIZE(strNormalFileName), TextureCount, TextureSuffix_Normal, false);
        CreateTextureFilename(strSpecularFileName, ARRAYSIZE(strSpecularFileName), TextureCount, TextureSuffix_Specular, false);
        if (!FileExists(strDiffuseFileName) || !FileExists(strNormalFileName) || !FileExists(strSpecularFileName))
        {
            Found = false;
        }
        else
        {
            ++TextureCount;
        }
    }

    g_SceneTextureCount = TextureCount;
}

void LocateTextures()
{
#if !defined(_XBOX_ONE)

#if _GAMING_DESKTOP || PC_BUILD
    WCHAR strParentPath[MAX_PATH];
    GetCurrentDirectory(ARRAYSIZE(strParentPath), strParentPath);
    wcscpy_s(g_strTextureBasePath, strParentPath);
#else
    {
        Windows::Storage::StorageFolder^ TempFolder = Windows::Storage::ApplicationData::Current->TemporaryFolder;
        const WCHAR* strPath = TempFolder->Path->ToString()->Begin();
        wcscpy_s(g_strTextureBasePath, strPath);
    }
#endif
    wcscat_s(g_strTextureBasePath, L"\\streamingtextures\\");
    DebugSpew("Texture path: %S\n", g_strTextureBasePath);

#endif

    ScanForTextures();

#if defined(_XBOX_ONE)
    if (g_SceneTextureCount == 0)
    {
        wcscpy_s(g_strTextureBasePath, g_strTextureBasePath2);
        ScanForTextures();
    }
#endif

#if !defined(_XBOX_ONE) && !defined(PC_BUILD)

    if (g_SceneTextureCount == 0)
    {
    #if _GAMING_DESKTOP || PC_BUILD
        ShellExecute(nullptr, L"explore", strParentPath, nullptr, nullptr, SW_SHOWNORMAL);
    #else
        Windows::System::Launcher::LaunchFolderAsync(Windows::Storage::ApplicationData::Current->TemporaryFolder);
    #endif
    }

#endif
}

void SetTextureBasePath(const WCHAR* strPath)
{
    wcscpy_s(g_strTextureBasePath, strPath);
}
const WCHAR* GetTextureBasePath() { return g_strTextureBasePath; }

UINT32 GetSceneTextureCount() { return g_SceneTextureCount; }

void CreateTextureFilename(WCHAR* strOutput, SIZE_T OutputSizeChars, UINT32 TextureIndex, TextureSuffix Suffix, bool IsDataFile)
{
    const WCHAR* strSuffix = nullptr;
    switch (Suffix)
    {
    case TextureSuffix_Diffuse:
        strSuffix = L"d";
        break;
    case TextureSuffix_Normal:
        strSuffix = L"n";
        break;
    case TextureSuffix_Specular:
        strSuffix = L"s";
        break;
    }

    const WCHAR* strExtension = IsDataFile ? L"trdata" : L"trimg";

    if (strSuffix != nullptr)
    {
        swprintf_s(strOutput, OutputSizeChars, L"%stexture%04u_%s.%s", g_strTextureBasePath, TextureIndex, strSuffix, strExtension);
    }
    else
    {
        swprintf_s(strOutput, OutputSizeChars, L"%stexture%04u.%s", g_strTextureBasePath, TextureIndex, strExtension);
    }
}
