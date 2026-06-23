//--------------------------------------------------------------------------------------
// File: WriteData.h
//
// Helper for writing binary data files to disk
//
// For Windows desktop apps, it creates files relative to the same folder as the running EXE if
// it can't create them relative to the CWD
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <exception>
#include <fstream>
#include <vector>

namespace DX
{
    inline void WriteData(_In_z_ const wchar_t* name, _In_ const void* data, _In_ size_t size)
    {
        std::ofstream outFile(name, std::ios::out | std::ios::binary | std::ios::trunc);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        if (!outFile)
        {
            wchar_t moduleName[_MAX_PATH] = {};
            if (!GetModuleFileNameW(nullptr, moduleName, _MAX_PATH))
                throw std::exception("GetModuleFileName");

            wchar_t drive[_MAX_DRIVE];
            wchar_t path[_MAX_PATH];

            if (_wsplitpath_s(moduleName, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0))
                throw std::exception("_wsplitpath_s");

            wchar_t filename[_MAX_PATH];
            if (_wmakepath_s(filename, _MAX_PATH, drive, path, name, nullptr))
                throw std::exception("_wmakepath_s");

            outFile.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
        }
#endif

        if (!outFile)
            throw std::exception("WriteData");

        outFile.write(reinterpret_cast<const char*>(data), std::streamsize(size));
        if (!outFile)
            throw std::exception("WriteData");

        outFile.close();
    }

    inline void WriteData(_In_z_ const wchar_t* name, _In_ const std::vector<uint8_t>& blob)
    {
        WriteData(name, blob.data(), blob.size());
    }

    inline void WriteData(_In_z_ const wchar_t* name, _In_ ID3DBlob* blob)
    {
        WriteData(name, blob->GetBufferPointer(), blob->GetBufferSize());
    }
}
