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

// DirectStorage supports the Decompression of pending read requests
// This sample shows how to use decompression and the various options
class ZLibDecompression : public ImplementationBase
{
public:
    ZLibDecompression() {}
    ~ZLibDecompression() {}

    bool RunSample(const std::wstring& fileName,uint32_t zipFileUncompressedSize);
};
