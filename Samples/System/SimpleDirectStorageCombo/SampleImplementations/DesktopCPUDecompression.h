//--------------------------------------------------------------------------------------
// DesktopCPUDecompression.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

#ifndef _GAMING_XBOX
// DirectStorage on Desktop supports title supplied callbacks for decompression, it currently does not include any native CPU decompression codecs
// This sample shows how to use title supplied callback for decompression on the desktop
class DesktopCPUDecompression : public ImplementationBase
{
private:
    size_t LZInflate(void* output, size_t outputSize, const void* input, size_t length);

public:
    DesktopCPUDecompression() = default;
    ~DesktopCPUDecompression() = default;

    bool RunSample(const std::wstring& fileName, uint32_t zipFileUncompressedSize);
};
#endif
