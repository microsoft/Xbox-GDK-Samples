//--------------------------------------------------------------------------------------
// CompletionEvent.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

#if ((_GRDK_VER >= 0x585D073C)&&(defined _GAMING_XBOX_SCARLETT)) || defined(_GAMING_DESKTOP) /* GDK Edition 221000 */
// DirectStorage supports three main methods of notification, status block, ID3D12Fence, and Windows Events
// This sample shows how to use a Windows Event for notification
// Using a Windows Event for notification is only supported on Xbox using the October 2022 GXDK or DirectStorage on PC
class CompletionEvent : public ImplementationBase
{
private:
    static const uint32_t c_dataBatches = 10;
    static const uint32_t c_readsPerBatch = 7;

    HANDLE m_completionEvent[c_dataBatches];

public:
    CompletionEvent() = default;
    ~CompletionEvent() = default;

    bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};

#endif
