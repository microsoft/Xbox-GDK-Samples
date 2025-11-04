//--------------------------------------------------------------------------------------
// FileReader.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DownloadableContent.h"
#include "FileReader.h"

extern std::unique_ptr<Sample> g_sample;

FileReader::FileReader() :
#ifndef _GAMING_XBOX_XBOXONE
    m_bUseWin32(false),
#endif
    m_hFile(INVALID_HANDLE_VALUE)
{
}

FileReader::~FileReader()
{
}

HRESULT FileReader::Open(const char* filename)
{
    if (!filename)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    // DirectStorage works only on Xbos Series and Desktop.

#ifndef _GAMING_XBOX_XBOXONE

    // In this sample, the DirectStorage objects are created every time for clarity,
    // but it should not be done in a real game. Factory and Queue can be reused many times.

    hr = DStorageGetFactory(IID_PPV_ARGS(&m_factory));

    if (SUCCEEDED(hr))
    {
        std::wstring filenameW = DX::Utf8ToWide(filename);
        hr = m_factory->OpenFile(filenameW.c_str(), IID_PPV_ARGS(&m_file));

        if (SUCCEEDED(hr))
        {
            DSTORAGE_QUEUE_DESC queueDesc = {};
            queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
            queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;
            queueDesc.Name = u8"realtime queue";

            hr = m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_queue));

            if (SUCCEEDED(hr))
            {
                hr = m_factory->CreateStatusArray(1, u8"Status Array", IID_PPV_ARGS(&m_status));

                if (FAILED(hr))
                {
                    g_sample->ErrorMessage("CreateStatusArray failed : 0x%08X\n", hr);
                }
            }
            else
            {
                g_sample->ErrorMessage("CreateQueue failed : 0x%08X\n", hr);
            }

            return hr;
        }
        else if (hr == E_DSTORAGE_XVD_DEVICE_NOT_SUPPORTED)
        {
            // DirectStorage doesn't support to read from HDD on Scarlett

            g_sample->ErrorMessage("OpenFile failed : 0x%08X. Fall back to Win32 File I/O\n", hr);
            m_bUseWin32 = true;
            hr = S_OK;
        }
        else
        {
            g_sample->ErrorMessage("OpenFile failed : 0x%08X\n", hr);
            return hr;
        }
    }
    else
    {
        g_sample->ErrorMessage("DStorageGetFactory failed : 0x%08X\n", hr);
        return hr;
    }
#endif

    m_hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        g_sample->ErrorMessage("Failed to open %s\n", filename);
        hr = E_HANDLE;
    }

    return hr;
}

HRESULT FileReader::GetSize(UINT* size) const
{
    if (!size)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    *size = 0;

#ifndef _GAMING_XBOX_XBOXONE
    if (m_bUseWin32)
    {
        FILE_STANDARD_INFO fileInfo = {};
        if (GetFileInformationByHandleEx(m_hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            *size = fileInfo.EndOfFile.LowPart;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        // DirectStorage
#ifdef _GAMING_XBOX_SCARLETT
        FILE_STANDARD_INFO fileInfo = {};
        HANDLE hFile = m_file->GetHandle();
        if (GetFileInformationByHandleEx(hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            *size = fileInfo.EndOfFile.LowPart;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        std::ignore = CloseHandle(hFile);
#else
        BY_HANDLE_FILE_INFORMATION fileinfo = {};
        hr = m_file->GetFileInformation(&fileinfo);
        if (SUCCEEDED(hr))
        {
            *size = fileinfo.nFileSizeLow;
        }
#endif
    }

    if (*size == 0)
    {
        g_sample->ErrorMessage("Failed to get the file size : 0x%08X\n", hr);
    }

    return hr;
#else
    FILE_STANDARD_INFO fileInfo = {};
    if (GetFileInformationByHandleEx(m_hFile, FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        *size = fileInfo.EndOfFile.LowPart;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
#endif
    return hr;
}

HRESULT FileReader::Read(void* buffer, UINT32 size)
{
    if (!buffer)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

#ifndef _GAMING_XBOX_XBOXONE
    if (m_bUseWin32)
    {
        DWORD bytesRead;
        if (ReadFile(m_hFile, buffer, size, &bytesRead, nullptr) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            g_sample->ErrorMessage("ReadFile Failed : 0x%08X\n", hr);
        }
    }
    else
    {
        DSTORAGE_REQUEST request = {};
#ifdef _GAMING_XBOX_SCARLETT
        request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        request.File = m_file.Get();
        request.Destination = buffer;
        request.FileOffset = 0;
        request.DestinationSize = size;
        request.SourceSize = size;
        request.CancellationTag = 0;
#else
        request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
        request.Source.File.Source = m_file.Get();
        request.Destination.Memory.Buffer = buffer;
        request.Source.File.Offset = 0;
        request.Source.File.Size = size;
        request.Destination.Memory.Size = size;
        request.UncompressedSize = size;
        request.CancellationTag = 0;
#endif
        m_queue->EnqueueRequest(&request);
        m_queue->EnqueueStatus(m_status.Get(), 0);
        m_queue->Submit();

        // Spin waiting until the first slot in the status array is marked complete
        while (!m_status->IsComplete(0))
        {
            Sleep(1);
        }
    }
#else
    DWORD bytesRead;
    if (ReadFile(m_hFile, buffer, size, &bytesRead, nullptr) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        g_sample->ErrorMessage("ReadFile Failed : 0x%08X\n", hr);
    }
#endif

    return hr;
}

void FileReader::Close()
{
#ifndef _GAMING_XBOX_XBOXONE
    if (m_bUseWin32 && m_hFile != INVALID_HANDLE_VALUE)
    {
        std::ignore = CloseHandle(m_hFile);
    }

    m_status.Reset();
    m_queue.Reset();
    m_file.Reset();
    m_factory.Reset();
#else
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        std::ignore = CloseHandle(m_hFile);
    }
#endif
}
