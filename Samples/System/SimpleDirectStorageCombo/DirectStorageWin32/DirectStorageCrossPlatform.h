//--------------------------------------------------------------------------------------
// DirectStorageCrossPlatform.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// This file is helper interface created for this sample to create a common interface for the functionality shown
// There are subtle differences between the DirectStorage interfaces on the console and on the Desktop
// The differences are based around the additional features that DirectStorage on desktop offers

// Included is an implementation of the console DirectStorage API set that uses only standard Win32 calls
// This implementation is used for the Xbox One family consoles which does not have native support for DirectStorage
// It's possible to force usage of this API set by setting the define FORCE_DS_WIN32
//#define FORCE_DS_WIN32

#if defined (_GAMING_XBOX_XBOXONE) || defined (FORCE_DS_WIN32)
#include "dstorage_win32.h"
#elif defined(_GAMING_XBOX_SCARLETT)
#include <dstorage_xs.h>
#else
#include <dstorage.h>
#endif

#if defined (_GAMING_XBOX_XBOXONE) || defined (FORCE_DS_WIN32)
#define DStorageFileCrossPlatform IDStorageFileWin32
#define DStorageFactoryCrossPlatform IDStorageFactoryWin32
#define DStorageStatusArrayCrossPlatform IDStorageStatusArrayWin32
#define DStorageQueueCrossPlatform  IDStorageQueueWin32
#elif defined (_GAMING_XBOX_SCARLETT)
#define DStorageFileCrossPlatform IDStorageFileX
#define DStorageStatusArrayCrossPlatform IDStorageStatusArrayX

#if _GRDK_VER >= 0x55F0113F /* GDK Edition 220603 */
#define DStorageFactoryCrossPlatform IDStorageFactoryX1
#else   // GDK edition not at least 220603
#define DStorageFactoryCrossPlatform IDStorageFactoryX
#endif  // check GDK edition for which factory to use

#if _GRDK_VER >= 0x585D073C /* GDK Edition 221000 */
#define DStorageQueueCrossPlatform  IDStorageQueueX1
#else   // GDK edition not at least 221000
#define DStorageQueueCrossPlatform  IDStorageQueueX
#endif  // check GDK edition for which queue to use

#else
#define DStorageFileCrossPlatform IDStorageFile
#define DStorageFactoryCrossPlatform IDStorageFactory
#define DStorageStatusArrayCrossPlatform IDStorageStatusArray
#define DStorageQueueCrossPlatform  IDStorageQueue1
#endif

inline HRESULT DStorageGetFactoryCrossPlatform(void** ppv) { return DStorageGetFactory(__uuidof(DStorageFactoryCrossPlatform), ppv); }

struct DStorageRequestCrossPlatform : public DSTORAGE_REQUEST
{
public:
    void SetupUncompressedRead(DStorageFileCrossPlatform* file, void* destBuffer, UINT64 fileOffset, UINT32 size, UINT64 cancellationTag = 0)
    {
#if defined (_GAMING_XBOX) || defined (FORCE_DS_WIN32)
        Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        File = file;
        Destination = destBuffer;
        FileOffset = fileOffset;
        DestinationSize = size;
        SourceSize = size;
        CancellationTag = cancellationTag;
#else
        Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
        Source.File.Source = file;
        Destination.Memory.Buffer = destBuffer;
        Source.File.Offset = fileOffset;
        Source.File.Size = size;
        Destination.Memory.Size = size;
        UncompressedSize = size;
        CancellationTag = cancellationTag;
#endif
    }

    void SetupXboxHardwareZLibRead(DStorageFileCrossPlatform* file, void* destBuffer, UINT64 fileOffset, UINT32 compressedSize, UINT32 uncompressedSize, UINT64 cancellationTag = 0)
    {
        SetupUncompressedRead(file, destBuffer, fileOffset, compressedSize, cancellationTag);
#if defined (_GAMING_XBOX) || defined (FORCE_DS_WIN32)
        DestinationSize = uncompressedSize;
        Options.ZlibDecompress = true;
#else
        UNREFERENCED_PARAMETER(file);
        UNREFERENCED_PARAMETER(destBuffer);
        UNREFERENCED_PARAMETER(fileOffset);
        UNREFERENCED_PARAMETER(compressedSize);
        UNREFERENCED_PARAMETER(uncompressedSize);
        UNREFERENCED_PARAMETER(cancellationTag);
        throw std::exception("Attempting to use Xbox hardware decompression from a Desktop application");
#endif
    }

    void SetupXboxHardwareInMemoryZLib(void* srcBuffer, void* destBuffer, UINT32 compressedSize, UINT32 uncompressedSize, UINT64 cancellationTag = 0)
    {
        SetupXboxHardwareZLibRead(nullptr, destBuffer, 0, compressedSize, uncompressedSize, cancellationTag);
#if defined (_GAMING_XBOX) || defined (FORCE_DS_WIN32)
        Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        Source = srcBuffer;
#else
        UNREFERENCED_PARAMETER(srcBuffer);
        UNREFERENCED_PARAMETER(destBuffer);
        UNREFERENCED_PARAMETER(compressedSize);
        UNREFERENCED_PARAMETER(uncompressedSize);
        UNREFERENCED_PARAMETER(cancellationTag);
        throw std::exception("Attempting to use Xbox hardware decompression from a Desktop application");
#endif
    }

    void SetupUncompressedRead(DStorageFileCrossPlatform* file, PULONG_PTR physicalPageArray, UINT64 fileOffset, UINT32 fileSize, UINT16 physicalOffset, UINT64 cancellationTag = 0)
    {
#if defined (_GAMING_XBOX) || defined (FORCE_DS_WIN32)
        SetupUncompressedRead(file, nullptr, fileOffset, fileSize, cancellationTag);
        Options.DestinationIsPhysicalPages = 1;
        DestinationPageArray = physicalPageArray;
        DestinationPageOffset = physicalOffset;
#else
        UNREFERENCED_PARAMETER(file);
        UNREFERENCED_PARAMETER(physicalPageArray);
        UNREFERENCED_PARAMETER(fileOffset);
        UNREFERENCED_PARAMETER(fileSize);
        UNREFERENCED_PARAMETER(physicalOffset);
        UNREFERENCED_PARAMETER(cancellationTag);
        throw std::exception("Attempting to use Xbox physical pages from a Desktop application");
#endif
    }

    void SetupXboxHardwareInMemoryZLib(void* srcBuffer, PULONG_PTR physicalPageArray, UINT32 compressedSize, UINT32 uncompressedSize, UINT16 physicalOffset, UINT64 cancellationTag = 0)
    {
#if defined (_GAMING_XBOX) || defined (FORCE_DS_WIN32)
        SetupXboxHardwareInMemoryZLib(srcBuffer, nullptr, compressedSize, uncompressedSize, cancellationTag);
        Options.DestinationIsPhysicalPages = 1;
        DestinationPageArray = physicalPageArray;
        DestinationPageOffset = physicalOffset;
#else
        UNREFERENCED_PARAMETER(srcBuffer);
        UNREFERENCED_PARAMETER(physicalPageArray);
        UNREFERENCED_PARAMETER(compressedSize);
        UNREFERENCED_PARAMETER(uncompressedSize);
        UNREFERENCED_PARAMETER(physicalOffset);
        UNREFERENCED_PARAMETER(cancellationTag);
        throw std::exception("Attempting to use Xbox hardware decompression from a Desktop application");
#endif
    }
};
