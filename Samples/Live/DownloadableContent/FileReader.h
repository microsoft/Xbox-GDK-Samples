//--------------------------------------------------------------------------------------
// FileReader.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class FileReader
{
public:
    FileReader();
    ~FileReader();

    FileReader(FileReader&&) = default;
    FileReader& operator= (FileReader&&) = default;

    FileReader(FileReader const&) = delete;
    FileReader& operator= (FileReader const&) = delete;

    HRESULT Open(const char* filename);
    HRESULT GetSize(UINT* size) const;
    HRESULT Read(void* buffer, UINT size);
    void Close();

private:
    std::unique_ptr<uint8_t[]> m_buffer;
#ifndef _GAMING_XBOX_XBOXONE
    bool m_bUseWin32;
#endif
    HANDLE m_hFile;

#ifdef _GAMING_XBOX_SCARLETT
    Microsoft::WRL::ComPtr<IDStorageFactoryX1>    m_factory;
    Microsoft::WRL::ComPtr<IDStorageFileX>        m_file;
    Microsoft::WRL::ComPtr<IDStorageQueueX1>      m_queue;
    Microsoft::WRL::ComPtr<IDStorageStatusArrayX> m_status;
#elif defined(_GAMING_XBOX_XBOXONE)
    // Xbox One doesn't support DirectStorage
#else
    // Windows
    Microsoft::WRL::ComPtr<IDStorageFactory>     m_factory;
    Microsoft::WRL::ComPtr<IDStorageFile>        m_file;
    Microsoft::WRL::ComPtr<IDStorageQueue>       m_queue;
    Microsoft::WRL::ComPtr<IDStorageStatusArray> m_status;
#endif
};
