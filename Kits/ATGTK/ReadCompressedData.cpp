//--------------------------------------------------------------------------------------
// File: ReadCompressedData.cpp
//
// Helper for loading binary data files from disk compressed with the
// Microsoft SZDD/KWAJ-style compression tool for Windows & Xbox.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "ReadCompressedData.h"

#include <cstddef>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

#ifndef NTDDI_WIN10_FE
#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) 1
#endif

#include <compressapi.h>

#ifndef NTDDI_WIN10_FE
#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) (Partitions)
#endif

namespace
{
    constexpr uint8_t c_CFileSignatureLen = 8;
    constexpr uint8_t c_CFileVersion = 0x41;

    const uint8_t c_Signature[c_CFileSignatureLen] = { 0x41, 0x46, 0x43, 0x57, 0x47, 0x50, 0x53, 0x4d };

#pragma pack(push,1)
    struct CFileHeader
    {
        uint8_t     magic[c_CFileSignatureLen]; // Must match c_Signature below
        uint8_t     mode;                       // COMPRESS_ALGORITHM_x enum
        uint8_t     version;
        wchar_t     lastChar;
        uint32_t    uncompressedSized;
    };
#pragma pack(pop)

    static_assert(sizeof(CFileHeader) == 16, "File header size mismatch");

    PVOID SimpleAlloc(PVOID, SIZE_T Size)
    {
        return malloc(Size);
    }

    VOID SimpleFree(PVOID, PVOID Memory)
    {
        free(Memory);
    }

    struct decompressor_closer { void operator()(void* h) { if (h) CloseDecompressor(static_cast<DECOMPRESSOR_HANDLE>(h)); } };

    HRESULT DecompressFile(
        _In_reads_bytes_(dataLen) const void* data,
        size_t dataLen,
        std::vector<uint8_t>& blob)
    {
        if (!data || !dataLen)
            return E_INVALIDARG;

        if (dataLen <= sizeof(CFileHeader))
            return E_FAIL;

        auto hdr = reinterpret_cast<const CFileHeader*>(data);
        if (memcmp(hdr, c_Signature, c_CFileSignatureLen) != 0)
            return E_FAIL;

        if (hdr->version != c_CFileVersion)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        switch (hdr->mode)
        {
        case COMPRESS_ALGORITHM_MSZIP:
        case COMPRESS_ALGORITHM_LZMS:
            break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        COMPRESS_ALLOCATION_ROUTINES allocData = { SimpleAlloc, SimpleFree, nullptr };

        std::unique_ptr<void, decompressor_closer> decompressor;
        {
            DECOMPRESSOR_HANDLE h = nullptr;
            if (!CreateDecompressor(
                static_cast<DWORD>(hdr->mode),
                &allocData,
                &h))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            decompressor.reset(h);
        }

        // Query decompressed buffer size.
        auto payload = reinterpret_cast<const void*>(reinterpret_cast<const uint8_t*>(data) + sizeof(CFileHeader));
        const size_t payloadLen = dataLen - sizeof(CFileHeader);

        SIZE_T bufferSize;
        if (!Decompress(
            static_cast<DECOMPRESSOR_HANDLE>(decompressor.get()),
            payload,
            payloadLen,
            nullptr,
            0,
            &bufferSize))
        {
            const DWORD errorCode = GetLastError();
            if (errorCode != ERROR_INSUFFICIENT_BUFFER)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        if (SIZE_T(hdr->uncompressedSized) != bufferSize)
            return E_FAIL;

        blob.resize(bufferSize);

        SIZE_T blobSize;
        if (!Decompress(
            static_cast<DECOMPRESSOR_HANDLE>(decompressor.get()),
            payload,
            payloadLen,
            blob.data(),
            bufferSize,
            &blobSize))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (blobSize > UINT32_MAX)
        {
            return E_FAIL;
        }

        blob.resize(blobSize);

        return S_OK;
    }
}

std::vector<uint8_t> DX::ReadCompressedData(_In_z_ const wchar_t* name)
{
    std::ifstream inFile(name, std::ios::in | std::ios::binary | std::ios::ate);
    if (!inFile)
    {
#ifdef _DEBUG
        wchar_t errorMessage[1024] = {};
        swprintf_s(errorMessage, 1024, L"ERROR: ReadCompressedData file not open %ls\n", name);
        OutputDebugStringW(errorMessage);
#endif
        throw std::runtime_error("ReadCompressedData");
    }

    const std::streampos compressedLen = inFile.tellg();
    if (!inFile)
        throw std::runtime_error("ReadCompressedData");

    auto compressedData = std::make_unique<uint8_t[]>(static_cast<size_t>(compressedLen));

    inFile.seekg(0, std::ios::beg);
    if (!inFile)
        throw std::runtime_error("ReadCompressedData");

    inFile.read(reinterpret_cast<char*>(compressedData.get()), compressedLen);
    if (!inFile)
        throw std::runtime_error("ReadCompressedData");

    inFile.close();

    std::vector<uint8_t> blob;

    if (FAILED(DecompressFile(compressedData.get(), static_cast<size_t>(compressedLen), blob)))
    {
        throw std::runtime_error("ReadCompressedData");
    }

    return blob;
}
