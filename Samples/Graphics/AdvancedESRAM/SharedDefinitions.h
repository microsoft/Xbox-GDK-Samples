//--------------------------------------------------------------------------------------
// SharedDefinitions.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace
{
    // Conversion functions
    template <typename T> constexpr T Kibibytes(T val) { return val * 1024u; }
    template <typename T> constexpr T Mebibytes(T val) { return Kibibytes(val * 1024u); }
    template <typename T> constexpr T Gibibytes(T val) { return Mebibytes(val * 1024u); }


    // Common Constants
    constexpr uint32_t c_pageSizeBytes = Kibibytes(64u);

                                                                                 // ESRAM Constants
    constexpr uint32_t c_esramSizeBytes = Mebibytes(32u);                        // 32 MiB
    constexpr uint32_t c_esramPageCount = c_esramSizeBytes / c_pageSizeBytes;

    // DRAM Constants
    constexpr uint32_t c_dramBlockPageCount = 64u;                               // 64 pages per DRAM block; somewhat arbitrarily chosen.
    constexpr uint32_t c_dramBlockSize = c_dramBlockPageCount * c_pageSizeBytes; // 4 MiB per DRAM block

    template <typename T>
    constexpr T DivRoundUp(T num, T denom)
    {
        return (num + denom - 1) / denom;
    }

    template <typename T>
    constexpr T PageCount(T byteSize)
    {
        return DivRoundUp(byteSize, T(c_pageSizeBytes));
    }
}

namespace ATG
{
    struct VMemDeleter
    {
        void operator()(void* mem) { VirtualFree(mem, 0, MEM_RELEASE); }
    };

    // Small helper type to perform virtual memory cleanup on destruction.
    using VirtualMemPtr = std::unique_ptr<void, VMemDeleter>;
}
