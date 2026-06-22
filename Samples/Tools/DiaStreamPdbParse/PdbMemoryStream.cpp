//--------------------------------------------------------------------------------------
// PdbMemoryStream.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PdbMemoryStream.h"

#ifndef _GAMING_DESKTOP
#include <xmem.h>
#endif

namespace
{
    constexpr size_t c_2GB = 2ULL * 1024 * 1024 * 1024;
}
// DIA supports the ability to read from a provided IStream interface, this replaces its own use of a memory mapped file internally.
// Providing an IStream object allows the title to control and cache the reads from the drive which can speed up PDB parsing on rotational drives
class InMemoryPdbStream : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom | Microsoft::WRL::RuntimeClassType::InhibitFtmBase>, IPdbMemoryStream, IStream>
{
public:
    ~InMemoryPdbStream()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
        }
        if (m_cache)
        {
#ifdef _GAMING_DESKTOP
            VirtualFree(m_cache, 0, MEM_RELEASE);
#else
            if (m_usingToolingMemory)
                VirtualFree(m_cache, 0, MEM_RELEASE);
            else
                XMemFree(m_cache, m_bufferAllocAttributes.Attributes);
#endif
            m_cache = nullptr;
        }
        delete[] m_cacheBlocks;
        m_cacheBlocks = nullptr;
    }

    HRESULT AllocateCacheMemory()
    {
        HRESULT hr = S_OK;
        if (m_cacheBlockSize == UINT64_MAX)
        {
            m_numCacheBlocks = 1;
            m_cacheBlockSize = m_fileLength;
            m_cacheBlocks = new CacheBlock[1];
            if (m_cacheBlocks == nullptr)
                return ERROR_NOT_ENOUGH_MEMORY;
#ifdef _GAMING_DESKTOP
            m_cache = static_cast<uint8_t*> (VirtualAlloc(nullptr, m_cacheBlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
            m_usingToolingMemory = true;
            m_cache = static_cast<uint8_t*> (XMemVirtualAlloc(nullptr, m_cacheBlockSize, MEM_COMMIT | MEM_RESERVE | MEM_2MB_PAGES, XMEM_CPU | XMEM_TOOL, PAGE_READWRITE));
            if (m_cache == nullptr)
            {
                m_usingToolingMemory = false;
                m_bufferAllocAttributes.Attributes = 0;
                m_bufferAllocAttributes.s.MemoryType = XALLOC_MEMTYPE_HEAP_CACHEABLE;
                m_bufferAllocAttributes.s.PageSize = XALLOC_PAGESIZE_2MB;
                m_bufferAllocAttributes.s.Alignment = XALLOC_ALIGNMENT_ANY;
                m_cache = static_cast<uint8_t*> (XMemAlloc(m_cacheBlockSize, m_bufferAllocAttributes.Attributes));
            }
#endif
            if (m_cache == nullptr)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                m_cacheBlocks[0].age = 0;
                m_cacheBlocks[0].filePosition = 0;
                uint64_t leftToRead = m_fileLength;
                uint64_t bufferIndex = 0;
                SetFilePointer(m_handle, 0, nullptr, FILE_BEGIN);
                while (leftToRead > 0)
                {
                    DWORD bytesActuallyRead;
                    uint32_t readThisIteration = 0;
                    if (leftToRead > c_2GB)		// can only read 2GB at a time using ReadFile
                        readThisIteration = c_2GB;
                    else
                        readThisIteration = static_cast<uint32_t> (leftToRead);

                    if (!ReadFile(m_handle, &(m_cache[bufferIndex]), readThisIteration, &bytesActuallyRead, NULL))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    else if (bytesActuallyRead != readThisIteration)
                    {
                        hr = E_UNEXPECTED;
                    }
                    leftToRead -= readThisIteration;
                    bufferIndex += readThisIteration;
                }
            }
        }
        else if (m_numCacheBlocks)
        {
            m_cacheBlocks = new CacheBlock[m_numCacheBlocks];
            if (m_cacheBlocks == nullptr)
                return ERROR_NOT_ENOUGH_MEMORY;
            for (uint32_t i = 0; i < m_numCacheBlocks; i++)
            {
                m_cacheBlocks[i].age = UINT64_MAX;      // marked as unused
            }
#ifdef _GAMING_DESKTOP
            m_cache = static_cast<uint8_t*> (VirtualAlloc(nullptr, m_numCacheBlocks*m_cacheBlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
            m_usingToolingMemory = true;
            m_cache = static_cast<uint8_t*> (XMemVirtualAlloc(nullptr, m_numCacheBlocks*m_cacheBlockSize, MEM_COMMIT | MEM_RESERVE | MEM_2MB_PAGES, XMEM_CPU | XMEM_TOOL, PAGE_READWRITE));
            if (m_cache == nullptr)
            {
                m_usingToolingMemory = false;
                m_bufferAllocAttributes.Attributes = 0;
                m_bufferAllocAttributes.s.MemoryType = XALLOC_MEMTYPE_HEAP_CACHEABLE;
                m_bufferAllocAttributes.s.PageSize = XALLOC_PAGESIZE_2MB;
                m_bufferAllocAttributes.s.Alignment = XALLOC_ALIGNMENT_ANY;
                m_cache = static_cast<uint8_t*> (XMemAlloc(m_numCacheBlocks*m_cacheBlockSize, m_bufferAllocAttributes.Attributes));
            }
#endif
            if (m_cache == nullptr)
            {                                                                      
                hr = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            m_cache = nullptr;
            m_cacheBlocks = nullptr;
            hr = S_OK;
        }
        if (!SUCCEEDED(hr))
        {
            if (m_cache)
            {
#ifdef _GAMING_DESKTOP
                VirtualFree(m_cache, 0, MEM_RELEASE);
#else
                if (m_usingToolingMemory)
                    VirtualFree(m_cache, 0, MEM_RELEASE);
                else
                    XMemFree(m_cache, m_bufferAllocAttributes.Attributes);
#endif
                m_cache = nullptr;
            }
            if (m_cacheBlocks)
            {
                delete[] m_cacheBlocks;
                m_cacheBlocks = nullptr;
            }
        }
        return hr;
    }

    uint32_t FindCacheSlot()
    {
        uint32_t minAgeSlot = 0;
        for (uint32_t i = 0; i < m_numCacheBlocks; i++)
        {
            if (m_cacheBlocks[i].age == UINT64_MAX)
                return i;
            if (m_cacheBlocks[minAgeSlot].age > m_cacheBlocks[i].age)
                minAgeSlot = i;
        }
        return minAgeSlot;
    }

    uint32_t FindUsedCacheSlot(uint64_t fileLocation)
    {
        uint64_t alignedBlockLocation = (fileLocation / m_cacheBlockSize) * m_cacheBlockSize;
        for (uint32_t i = 0; i < m_numCacheBlocks; i++)
        {
            if (m_cacheBlocks[i].age == UINT64_MAX)
                continue;
            if (m_cacheBlocks[i].filePosition == alignedBlockLocation)
                return i;
        }
        return UINT32_MAX;
    }

    HRESULT RuntimeClassInitialize(const wchar_t* pdbPath, uint64_t numCacheBlocks, uint64_t cacheBlockSize)
    {
        m_numCacheBlocks = numCacheBlocks;
        m_cacheBlockSize = cacheBlockSize;
        if (m_numCacheBlocks == 0)
            m_cacheBlockSize = 0;
        if (m_cacheBlockSize != UINT64_MAX)
            m_cacheBlockSize = (m_cacheBlockSize + (c_blockSizeAlignment - 1)) & ~(c_blockSizeAlignment - 1);
        m_cache = nullptr;
        m_cacheBlocks = nullptr;

        HRESULT hr = S_OK;

        m_handle = CreateFile(pdbPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (m_handle == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            LARGE_INTEGER fileSize{};
            if (!GetFileSizeEx(m_handle, &fileSize))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                m_fileLength = static_cast<uint64_t> (fileSize.QuadPart);
                m_filePath = pdbPath;
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = AllocateCacheMemory();
        }

        return hr;
    }
    IFACEMETHODIMP Read(_Out_writes_bytes_to_(bytesToRead, *bytesRead) void* destBuffer,
        _In_ ULONG bytesToRead,
        _Out_opt_ ULONG* bytesRead)
    {
        if (bytesRead != nullptr)
            *bytesRead = 0;

        if (m_cache == nullptr)
        {
            DWORD bytesActuallyRead;

            if (!ReadFile(m_handle, destBuffer, bytesToRead, &bytesActuallyRead, NULL))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
            else if (bytesToRead != bytesActuallyRead)
            {
                return E_UNEXPECTED;
            }
            if (bytesRead)
                *bytesRead = bytesActuallyRead;
        }
        else
        {
            uint32_t remainingLength = bytesToRead;
            uint64_t titleBlockReadOffset = m_readPosition % m_cacheBlockSize;
            uint32_t titleBlockReadLength = static_cast<uint32_t> (m_cacheBlockSize - titleBlockReadOffset);
            if (titleBlockReadLength > remainingLength)
                titleBlockReadLength = remainingLength;

            uint8_t *curDestination = static_cast<uint8_t*>(destBuffer);
            while (remainingLength)
            {
                uint32_t curCacheBlock = FindUsedCacheSlot(m_readPosition);
                if (curCacheBlock == UINT32_MAX)
                {
                    curCacheBlock = FindCacheSlot();
                    uint64_t alignedReadLocation = (m_readPosition / m_cacheBlockSize) * m_cacheBlockSize;
                    m_cacheBlocks[curCacheBlock].filePosition = alignedReadLocation;
                    DWORD bytesActuallyRead = 0;
                    uint32_t bytesToReadThisLoopIteration = static_cast<uint32_t> (m_cacheBlockSize);
                    if ((alignedReadLocation + bytesToReadThisLoopIteration) > m_fileLength)
                        bytesToReadThisLoopIteration = static_cast<uint32_t> (m_fileLength - alignedReadLocation);

                    LONG lowValue, highValue;
                    lowValue = static_cast<LONG> (alignedReadLocation & 0xffffffff);
                    highValue = static_cast<LONG> ((alignedReadLocation >> 32ULL) & 0xffffffff);

                    if (SetFilePointer(m_handle, lowValue, &highValue, SEEK_SET) == INVALID_SET_FILE_POINTER)
                        return HRESULT_FROM_WIN32(GetLastError());

                    if (!ReadFile(m_handle, &m_cache[curCacheBlock*m_cacheBlockSize], bytesToReadThisLoopIteration, &bytesActuallyRead, NULL))
                        return HRESULT_FROM_WIN32(GetLastError());

                    if (bytesToReadThisLoopIteration != bytesActuallyRead)
                        DebugBreak();
                    assert(bytesToReadThisLoopIteration == bytesActuallyRead);
                }
                m_cacheBlocks[curCacheBlock].age = m_currentAgeValue++;
                memcpy_s(curDestination, titleBlockReadLength, &m_cache[(curCacheBlock*m_cacheBlockSize) + titleBlockReadOffset], titleBlockReadLength);
                if (bytesRead != nullptr)
                    *bytesRead += titleBlockReadLength;
                remainingLength -= titleBlockReadLength;
                curDestination += titleBlockReadLength;
                m_readPosition += titleBlockReadLength;

                if (remainingLength < m_cacheBlockSize)
                    titleBlockReadLength = remainingLength;
                else
                    titleBlockReadLength = static_cast<uint32_t> (m_cacheBlockSize);
                titleBlockReadOffset = 0;
            }
        }

        return S_OK;
    }

    // IStream
    IFACEMETHODIMP Seek(LARGE_INTEGER distanceToMove,
        DWORD origin,
        _Out_opt_ ULARGE_INTEGER* newPosition)
    {
        HRESULT hr = S_OK;
        switch (origin)
        {
        case STREAM_SEEK_SET:
            m_readPosition = static_cast<uint64_t> (distanceToMove.QuadPart);
            break;
        case STREAM_SEEK_CUR:
            m_readPosition += distanceToMove.QuadPart;
            break;
        case STREAM_SEEK_END:
            m_readPosition = m_fileLength;
            m_readPosition += distanceToMove.QuadPart;
            break;
        default:
            hr = E_INVALIDARG;
            break;
        }
        if (m_cache == nullptr)
        {
            LONG lowValue, highValue;
            lowValue = static_cast<LONG> (m_readPosition & 0xffffffff);
            highValue = static_cast<LONG> ((m_readPosition >> 32ULL) & 0xffffffff);
            if (SetFilePointer(m_handle, lowValue, &highValue, SEEK_SET) == INVALID_SET_FILE_POINTER)
                hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if (SUCCEEDED(hr))
        {
            if (newPosition)
            {
                newPosition->QuadPart = m_readPosition;
            }
        }
        return hr;
    }

    IFACEMETHODIMP Stat(__RPC__out STATSTG* stats,
        DWORD grfStatFlag)
    {
        HRESULT hr = S_OK;

        if (stats == nullptr)
        {
            return E_INVALIDARG;
        }

        ZeroMemory(stats, sizeof(STATSTG));

        stats->type = STGTY_STREAM;
        stats->cbSize.QuadPart = m_fileLength;

        if (grfStatFlag == STATFLAG_DEFAULT)
        {
            size_t bufferPathSize = (m_filePath.size() + 1) * sizeof(WCHAR);
            PVOID pathBuffer = CoTaskMemAlloc(bufferPathSize);
            if (!pathBuffer)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memcpy_s(pathBuffer, bufferPathSize, m_filePath.c_str(), m_filePath.size() * sizeof(WCHAR));
                ((WCHAR*)pathBuffer)[m_filePath.size()] = L'\0';
                stats->pwcsName = reinterpret_cast<OLECHAR*>(pathBuffer);
            }
        }

        return hr;
    }

    IFACEMETHODIMP Clone(__RPC__deref_out_opt IStream** ppstm)
    {
        HRESULT hr = S_OK;
        UNREFERENCED_PARAMETER(ppstm);
        hr = E_NOTIMPL;

        return hr;
    }
    IFACEMETHODIMP Write(_In_reads_bytes_(bytesToWrite) const void* srcBuffer,
        _In_ ULONG bytesToWrite,
        _Out_opt_ ULONG* bytesWritten)
    {
        UNREFERENCED_PARAMETER(srcBuffer);
        UNREFERENCED_PARAMETER(bytesToWrite);
        UNREFERENCED_PARAMETER(bytesWritten);
        return E_NOTIMPL;
    }
    IFACEMETHODIMP SetSize(ULARGE_INTEGER newSize)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(newSize);
        hr = E_NOTIMPL;

        return hr;
    }

    IFACEMETHODIMP CopyTo(_In_ IStream* pstm,
        ULARGE_INTEGER cb,
        _Out_opt_ ULARGE_INTEGER* pcbRead,
        _Out_opt_ ULARGE_INTEGER* pcbWritten)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(pstm);
        UNREFERENCED_PARAMETER(cb);
        UNREFERENCED_PARAMETER(pcbRead);
        UNREFERENCED_PARAMETER(pcbWritten);
        hr = E_NOTIMPL;

        return hr;
    }

    IFACEMETHODIMP Commit(DWORD grfCommitFlags)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(grfCommitFlags);
        hr = E_NOTIMPL;

        return hr;
    }

    IFACEMETHODIMP Revert(void)
    {
        HRESULT hr = S_OK;
        hr = E_NOTIMPL;

        return hr;
    }

    IFACEMETHODIMP LockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(libOffset);
        UNREFERENCED_PARAMETER(cb);
        UNREFERENCED_PARAMETER(dwLockType);
        hr = E_NOTIMPL;

        return hr;
    }

    IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb,
        DWORD dwLockType)
    {
        HRESULT hr = S_OK;

        UNREFERENCED_PARAMETER(libOffset);
        UNREFERENCED_PARAMETER(cb);
        UNREFERENCED_PARAMETER(dwLockType);
        hr = E_NOTIMPL;

        return hr;
    }

private:
    static const uint64_t c_blockSizeAlignment = 4096;
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    std::wstring m_filePath;
    uint64_t m_readPosition = 0;
    uint64_t m_fileLength = 0;
    uint64_t m_numCacheBlocks = 0;
    uint64_t m_cacheBlockSize = 0;
    uint64_t m_currentAgeValue = 0;

    uint8_t *m_cache = nullptr;
    struct CacheBlock
    {
        uint64_t filePosition = 0;
        uint64_t age = UINT64_MAX;           // UINT64_MAX means unused
    };
    CacheBlock *m_cacheBlocks = nullptr;
#ifndef _GAMING_DESKTOP
    bool m_usingToolingMemory = false;
    XALLOC_ATTRIBUTES m_bufferAllocAttributes = {};
#endif
};

STDAPI CreatePdbMemoryStream(const wchar_t* pdbPath, uint64_t numCacheBlocks, uint64_t cacheBlockSize, REFIID riid, __deref_out void** ppvObject)
{
    if (!ppvObject || !pdbPath)
    {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    using namespace Microsoft::WRL;
    ComPtr<IPdbMemoryStream> impl;

    // Create the object
    HRESULT hr = MakeAndInitialize<InMemoryPdbStream>(&impl, pdbPath, numCacheBlocks, cacheBlockSize);
    if (SUCCEEDED(hr))
    {
        // Query the requested interface
        hr = impl->QueryInterface(riid, ppvObject);
    }

    return hr;
}
