//--------------------------------------------------------------------------------------
// util.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include <stdio.h>
#include "util.hpp"

void AssertImplementation(bool cond, const CHAR* strFormat, ...)
{
    if (cond)
    {
        return;
    }

    CHAR strLine[256];
    va_list args = NULL;

    va_start(args, strFormat);

    vsprintf_s(strLine, sizeof(strLine), strFormat, args);

    va_end(args);

    OutputDebugStringA("ASSERT: ");
    OutputDebugStringA(strLine);

    __debugbreak();
}

VOID DebugSpew( const CHAR* strFormat, ... )
{
    CHAR strLine[256];
    va_list args = NULL;

    va_start( args, strFormat );

    vsprintf_s( strLine, sizeof(strLine), strFormat, args );

    va_end( args );

    OutputDebugStringA( strLine );
}

HRESULT LoadFile( const WCHAR* strFileName, void** ppBuffer, UINT32* pBufferSizeBytes )
{
    HANDLE hFile = CreateFile2( strFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return E_FAIL;
    }

    LARGE_INTEGER FileSize;
    BOOL Success = GetFileSizeEx( hFile, &FileSize );
    if (!Success)
    {
        CloseHandle( hFile );
        return E_FAIL;
    }

    if (pBufferSizeBytes != nullptr)
    {
        *pBufferSizeBytes = (UINT32)FileSize.QuadPart;
    }

    if (ppBuffer == nullptr)
    {
        return S_OK;
    }

    void* pFileData = nullptr;
    bool LocalAlloc = false;
    if (*ppBuffer == nullptr)
    { 
        pFileData = malloc(FileSize.QuadPart);
        if (pFileData == nullptr)
        {
            CloseHandle(hFile);
            return E_OUTOFMEMORY;
        }
        LocalAlloc = true;
    }
    else
    {
        pFileData = *ppBuffer;
    }

    DWORD BytesRead = 0;
    Success = ReadFile( hFile, pFileData, (DWORD)FileSize.QuadPart, &BytesRead, NULL );
    if (!Success)
    {
        if (LocalAlloc)
        {
            free(pFileData);
        }
        CloseHandle(hFile);
        return E_FAIL;
    }

    CloseHandle(hFile);

    *ppBuffer = pFileData;

    return S_OK;
}

void UnloadFile( void* pBuffer )
{
    free(pBuffer);
}
