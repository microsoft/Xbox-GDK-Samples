//--------------------------------------------------------------------------------------
// File: OpenSSLHelperts.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <memory>

#include <openssl/ssl.h>
#include <openssl/bio.h>

namespace ATG
{
    
    template <typename T>
    struct deleter
    {
    };

    template <>
    struct deleter<SSL>
    {
        void operator()(SSL* ptr)
        {
            SSL_free(ptr);
        }
    };

    using SSL_PTR = std::unique_ptr<SSL, deleter<SSL>>;
}
