//--------------------------------------------------------------------------------------
// File: DtlsCreateConnectionAsyncOp.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "DtlsConnection.h"

#include "DtlsCreateConnectionAsyncOp.h"

namespace ATG
{
    HRESULT DtlsCreateConnectionAsyncOp::DoWork() noexcept
    {

        return S_OK;
    }

    HRESULT DtlsCreateConnectionAsyncOp::GetResult(const XAsyncProviderData* data) noexcept
    {
        if (data->bufferSize < resultSize)
        {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        auto ptr = connection.get();
        memcpy(data->buffer, &ptr, resultSize);
        return S_OK;
    }

    void DtlsCreateConnectionAsyncOp::Complete(HRESULT hr) noexcept
    {
        if (async)
        {
            XAsyncComplete(async, hr, SUCCEEDED(hr) ? resultSize : 0);
        }
    }

    HRESULT DtlsCreateConnectionAsyncOp::Provider(
        _In_ XAsyncOp op,
        _In_ const XAsyncProviderData* data
    ) noexcept
    {
        auto context = static_cast<DtlsCreateConnectionAsyncOp*>(data->context);

        switch (op)
        {
        case XAsyncOp::Begin:
            context->async = data->async;
            break;
        case XAsyncOp::Cleanup:
        {
            auto connection = context->connection;

            if (connection)
            {
                context->connection = nullptr;
                connection->m_createConnectionOp = nullptr;
            }
        }
        break;

        case XAsyncOp::GetResult:
        {
            HRESULT hr = context->GetResult(data);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        break;

        case XAsyncOp::Cancel:
        {
            XAsyncComplete(data->async, E_ABORT, 0);
        }
        break;
        case XAsyncOp::DoWork:
        break;
        }

        return S_OK;
    }
}
