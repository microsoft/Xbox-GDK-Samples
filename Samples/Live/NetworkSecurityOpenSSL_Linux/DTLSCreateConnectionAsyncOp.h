//--------------------------------------------------------------------------------------
// File: DTLSCreateConnectionAsyncOp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <memory>
#include "XAsync.h"
#include "XAsyncProvider.h"

namespace ATG
{
    struct DTLSConnection;

    struct DTLSCreateConnectionAsyncOp
    {
        size_t resultSize{ sizeof(DTLSConnection*) };
        XAsyncBlock* async;
        std::shared_ptr<DTLSConnection> connection;

        HRESULT DoWork() noexcept;

        HRESULT GetResult(const XAsyncProviderData* data) noexcept;

        void Complete(HRESULT hr) noexcept;

        static HRESULT Provider(
            _In_ XAsyncOp op,
            _In_ const XAsyncProviderData* data
        ) noexcept;
    };
}
