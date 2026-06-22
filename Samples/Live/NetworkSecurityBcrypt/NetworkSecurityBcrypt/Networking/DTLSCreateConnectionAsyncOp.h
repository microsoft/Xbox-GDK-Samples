//--------------------------------------------------------------------------------------
// File: DtlsCreateConnectionAsyncOp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <memory>

#include <XAsync.h>
#include <XAsyncProvider.h>

namespace ATG
{
    struct DtlsConnection;

    struct DtlsCreateConnectionAsyncOp
    {
        size_t resultSize{ sizeof(DtlsConnection*) };
        XAsyncBlock* async = nullptr;
        std::shared_ptr<DtlsConnection> connection;

        HRESULT DoWork() noexcept;

        HRESULT GetResult(const XAsyncProviderData* data) noexcept;

        void Complete(HRESULT hr) noexcept;

        static HRESULT Provider(
            _In_ XAsyncOp op,
            _In_ const XAsyncProviderData* data
        ) noexcept;
    };
}
