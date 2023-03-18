//--------------------------------------------------------------------------------------
// DesktopGPUDecompression.h
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
class DesktopGPUDecompression : public ImplementationBase
{
private:

    std::atomic<bool> m_success;
    std::wstring m_fileName;

public:
    DesktopGPUDecompression() = default;
    ~DesktopGPUDecompression() = default;

    bool RunSample(const std::wstring& fileName, ID3D12Device* device);
};
#endif
