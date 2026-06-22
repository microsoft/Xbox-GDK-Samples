//--------------------------------------------------------------------------------------
// File: SocketPayload.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

namespace ATG
{
    constexpr size_t c_MaxPayloadSize = 1384llu;

    struct SocketPayload
    {
        uint32_t size{ 0 };
        uint8_t  payload[c_MaxPayloadSize]{};
    };
}
