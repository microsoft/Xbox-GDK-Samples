//--------------------------------------------------------------------------------------
// IGenerator.h
//
// Common interface for different generator types
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <Windows.h>

namespace ATG
{
    class IGenerator
    {
        public:
            virtual ~IGenerator() {};
            virtual bool     IsEOF() const = 0;
            virtual uint32_t GetBufferLength() const = 0;
            virtual void     Flush() = 0;
            virtual HRESULT  FillSampleBuffer(uint32_t bytesToRead, uint8_t* pData) = 0;
    };
}
