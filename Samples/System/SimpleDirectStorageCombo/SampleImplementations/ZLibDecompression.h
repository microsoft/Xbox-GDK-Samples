//--------------------------------------------------------------------------------------
// Decompression.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// DirectStorage on the console is the way to access the hardware decompression engine
// It supports the zlib format, https://www.ietf.org/rfc/rfc1950.txt, as well as the Microsoft BCPack format
// This sample shows how to use decompression for data being read off the disk
class ZLibDecompression : public ImplementationBase
{
public:
    ZLibDecompression() {}
    ~ZLibDecompression() {}

    bool RunSample(const std::wstring& fileName, uint32_t zipFileUncompressedSize);
};
