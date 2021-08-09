//--------------------------------------------------------------------------------------
// File: FileDownloader.cpp
//
// Download and cache files from URLs
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"

#include "FileDownloader.h"

#include <xcurl.h>

using namespace ATG;

size_t WriteCallback(void *contents, size_t size, size_t count, void *context)
{
    size_t numbytes = size * count;
    Bytes* bytes = static_cast<Bytes*>(context);

    uint8_t* b = static_cast<uint8_t*>(contents);
    uint8_t* e = b + numbytes;
    std::copy(b, e, std::back_inserter(*bytes));

    return numbytes;
}

FileDownloader::FileDownloader()
{
    curl_global_init(0);
}

FileDownloader::~FileDownloader()
{
    curl_global_cleanup();
}

FileHandle FileDownloader::DownloadFile(const char* uri, const char* key)
{
    CURL *curl;

    Bytes bytes;

    CURLcode res;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, uri);
        curl_easy_setopt(curl, CURLOPT_HEADER, false);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&bytes));
        res = curl_easy_perform(curl);

#if (_GXDK_VER < 0x4A611B35 /* GXDK Edition 210600 */)
        // In one or more versions of xcurl prior to 2106, there was a bug where the header was returned regardless of other settings
        // To work around, check if the downloaded length is larger than the content size and trim the beginning if necessary
        curl_off_t contentLength;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);

        if (bytes.size() > static_cast<size_t>(contentLength))
        {
            uint32_t headersize;
            curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &headersize);
            bytes.erase(bytes.begin(), bytes.begin() + headersize);
        }
#endif
        

        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            m_files[key] = bytes;
        }
    }

    auto it = m_files.find(key);

    return (it != m_files.end()) ? &(it->second) : nullptr;
}

FileHandle FileDownloader::GetFile(const std::string& key)
{
    auto it = m_files.find(key);
    return (it != m_files.end()) ? &(it->second) : nullptr;
};

HRESULT FileDownloader::DownloadFileAsync(const std::string& uri, const std::string& key, XAsyncBlock* async)
{
    struct CallData
    {
        std::string uri;
        std::string key;
        FileHandle result;
        FileDownloader& downloader;
    };

    CallData* callData = new CallData{ uri, key, nullptr, *this };

    return XAsyncBegin(async, callData, nullptr, __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* providerData)
    {
        CallData* callData = reinterpret_cast<CallData*>(providerData->context);
        auto& downloader = callData->downloader;

        switch (op)
        {
        case XAsyncOp::Begin:
            return XAsyncSchedule(providerData->async, 0);

        case XAsyncOp::Cleanup:
            delete callData;
            break;

        case XAsyncOp::GetResult:
            memcpy_s(providerData->buffer, sizeof(FileHandle), &callData->result, sizeof(FileHandle));
            break;

        case XAsyncOp::DoWork:
            callData->result = downloader.DownloadFile(callData->uri.c_str(), callData->key.c_str());

            XAsyncComplete(providerData->async, callData->result != nullptr ? S_OK : E_FAIL, sizeof(FileHandle));
            break;

        case XAsyncOp::Cancel:
            // Can't be cancelled
            break;
        }

        return S_OK;
    });
}

HRESULT FileDownloader::DownloadFileAsyncResult(XAsyncBlock* async, _Out_ FileHandle* fileHandle)
{
    if (async == nullptr || fileHandle == nullptr)
    {
        return E_INVALIDARG;
    }

    return XAsyncGetResult(async, nullptr, sizeof(FileHandle), fileHandle, nullptr);
}
