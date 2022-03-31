//--------------------------------------------------------------------------------------
// InMemoryZLibDecompression.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "zlib/include/zlib.h"
#include "ImplementationBase.h"

#ifdef _GAMING_XBOX_SCARLETT
// DirectStorage on the console is the way to access the hardware decompression engine
// It supports the zlib format, https://www.ietf.org/rfc/rfc1950.txt, as well as the Microsoft BCPack format
// This sample is for the console only and shows how to use decompression for data already in memory
class InMemoryZLibDecompression : public ImplementationBase
{
private:
    static const uint32_t s_1KB = 1024;
    static const uint32_t s_1MB = 1024 * 1024;
    static const uint32_t c_numBlocks = 14;
    std::vector<std::unique_ptr<char[]>> m_dataBlocks;
    std::vector<std::pair<uint32_t, uint32_t>> m_dataBlockSizes;

    Microsoft::WRL::ComPtr <DStorageStatusArrayCrossPlatform> m_status;

    void CreateDataBlocks();

public:
    InMemoryZLibDecompression() {}
    ~InMemoryZLibDecompression() {}

    bool RunSample();
};
#endif
