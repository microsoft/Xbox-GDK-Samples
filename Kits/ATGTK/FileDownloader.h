//--------------------------------------------------------------------------------------
// File: FileDownloader.h
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

#pragma once

using Bytes = std::vector<uint8_t>;
using FileHandle = Bytes * ;

namespace ATG
{
    class FileDownloader
    {
    public:
        FileDownloader();
        ~FileDownloader();

        FileDownloader(FileDownloader&&) = delete;
        FileDownloader& operator= (FileDownloader&&) = delete;

        FileDownloader(FileDownloader const&) = delete;
        FileDownloader& operator= (FileDownloader const&) = delete;

        HRESULT DownloadFileAsync(const std::string& uri, const std::string& key, XAsyncBlock* async);
        HRESULT DownloadFileAsyncResult(XAsyncBlock* async, _Out_ FileHandle* fileHandle);

        FileHandle GetFile(const std::string& tag);

    private:
        

        FileHandle DownloadFile(const char* uri, const char* key);

        std::map<std::string, Bytes> m_files;
        std::mutex m_curlLock;
    };
}
